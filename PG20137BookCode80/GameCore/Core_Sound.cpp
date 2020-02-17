/**************************************************
WinMain.cpp
GameCore Component

Programming Role-Playing Games with DirectX
by Jim Adams (01 Jan 2002)
**************************************************/

#include "Core_Global.h"

//////////////////////////////////////////////////////////////////////
//
// cSound Functions
//
//////////////////////////////////////////////////////////////////////
cSound::cSound()
{
  // Initialize COM
  CoInitialize(NULL);

  m_hWnd   = NULL;
  m_Volume = 0;

  m_pDMPerformance = NULL;
  m_pDMLoader      = NULL;
}

cSound::~cSound()
{
  Shutdown();

  // Uninitialize COM
  CoUninitialize();
}

BOOL cSound::Init(HWND hWnd, long Frequency)
{
  CHAR             strPath[MAX_PATH];
  WCHAR            wstrSearchPath[MAX_PATH];
  DMUS_AUDIOPARAMS dmap;
  
  // Shutdown system in case of prior install
  Shutdown();

  // Save parent window handle
  if((m_hWnd = hWnd) == NULL)
    return FALSE;

  ///////////////////////////////////////////////////////////////////
  // Initialize DirectMusic
  ///////////////////////////////////////////////////////////////////

  // Create the DirectMusic loader object
  CoCreateInstance(CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, 
                   IID_IDirectMusicLoader8, (void**)&m_pDMLoader);

  // Create the DirectMusic performance object
  CoCreateInstance(CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC, 
                   IID_IDirectMusicPerformance8, (void**)&m_pDMPerformance);

  // Setup the playback format structure
  ZeroMemory(&dmap, sizeof(DMUS_AUDIOPARAMS));
  dmap.dwSize = sizeof(DMUS_AUDIOPARAMS);
  dmap.fInitNow = TRUE;
  dmap.dwValidData = DMUS_AUDIOPARAMS_FEATURES | DMUS_AUDIOPARAMS_SAMPLERATE;
  dmap.dwFeatures = DMUS_AUDIOF_ALL;
  dmap.dwSampleRate = Frequency;

  // Initialize the performance with the standard audio path.
  // This initializes both DirectMusic and DirectSound and 
  // sets up the synthesizer. 
  m_pDMPerformance->InitAudio(NULL, NULL, m_hWnd,
                              DMUS_APATH_SHARED_STEREOPLUSREVERB, 
                              64, 
                              DMUS_AUDIOF_ALL, &dmap);

  // Tell DirectMusic where the default search path is
  GetCurrentDirectory(MAX_PATH, strPath);
  MultiByteToWideChar(CP_ACP, 0, strPath, -1, wstrSearchPath, MAX_PATH);
  m_pDMLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, wstrSearchPath, FALSE);

  // Set default volume to full
  SetVolume(100);
  
  return TRUE;
}

BOOL cSound::Shutdown()
{
  // Stop the music, and close down 
  if(m_pDMPerformance != NULL) {
    m_pDMPerformance->Stop(NULL, NULL, 0, 0);
    m_pDMPerformance->CloseDown();
  }

  // Release the DirectMusic objects
  ReleaseCOM(m_pDMPerformance);
  ReleaseCOM(m_pDMLoader);

  return TRUE;
}

IDirectMusicPerformance8 *cSound::GetPerformanceCOM()
{
  return m_pDMPerformance;
}

IDirectMusicLoader8 *cSound::GetLoaderCOM()
{
  return m_pDMLoader;
}

long cSound::GetVolume()
{
  return m_Volume;
}

BOOL cSound::SetVolume(long Percent)
{
  long Volume;

  if(m_pDMPerformance == NULL)
    return FALSE;

  // Calculate a usable volume level from -90dB to +10dB
  Volume = ((Percent % 101) - 90) * 100;
  if(FAILED(m_pDMPerformance->SetGlobalParam(GUID_PerfMasterVolume, &Volume, sizeof(long))))
    return FALSE;

  // Save the volume level
  m_Volume = Percent % 101;

  return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// cSoundChannel Functions
//
//////////////////////////////////////////////////////////////////////
cSoundChannel::cSoundChannel()
{
  m_Sound      = NULL;
  m_pDMPath    = NULL;
  m_pDMSegment = NULL;

  m_Volume     = 0;
}

cSoundChannel::~cSoundChannel()
{
  Free();

  // Set to no parent sound
  m_Sound = NULL;
}

IDirectMusicAudioPath *cSoundChannel::GetAudioPathCOM()
{
  return m_pDMPath;
}

IDirectMusicSegment8 *cSoundChannel::GetSegmentCOM()
{
  return m_pDMSegment;
}

BOOL cSoundChannel::Create(cSound *Sound)
{
  // Free a prior channel
  Free();

  // Error checking
  if((m_Sound = Sound) == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;
  if(m_Sound->GetLoaderCOM() == NULL)
    return FALSE;

  // Create an audio path
  if(FAILED(m_Sound->GetPerformanceCOM()->CreateStandardAudioPath(DMUS_APATH_DYNAMIC_STEREO, 1, TRUE, &m_pDMPath)))
    return FALSE;

  return TRUE;
}

BOOL cSoundChannel::Free()
{
  // Stop any playback
  Stop();

  // Free the audio path
  ReleaseCOM(m_pDMPath);

  return TRUE;
}

BOOL cSoundChannel::Play(char *Filename, long VolumePercent, long Loop)
{
  DMUS_OBJECTDESC dmod;

  // Stop and free any playback
  Stop();

  // Get the object
  ZeroMemory(&dmod, sizeof(DMUS_OBJECTDESC));
  dmod.dwSize = sizeof(DMUS_OBJECTDESC);
  dmod.guidClass = CLSID_DirectMusicSegment;
  dmod.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
  mbstowcs(dmod.wszFileName, Filename, MAX_PATH);
  if(FAILED(m_Sound->GetLoaderCOM()->GetObject(&dmod, IID_IDirectMusicSegment8, (LPVOID*)&m_pDMSegment)))
    return FALSE;

  // Download the segment's instruments to the audio path
  m_pDMSegment->Download(m_pDMPath);

  // Set looping data
  m_Loop = Loop;
  if(!m_Loop)
    m_pDMSegment->SetRepeats(DMUS_SEG_REPEAT_INFINITE);
  else
    m_pDMSegment->SetRepeats(m_Loop-1);

  // Set the volume
  SetVolume(VolumePercent);

  // Start playing
  if(FAILED(m_Sound->GetPerformanceCOM()->PlaySegmentEx(m_pDMSegment, NULL, NULL, DMUS_SEGF_SECONDARY, 0, NULL, m_pDMPath, m_pDMPath)))
    return FALSE;

  return TRUE;
}

BOOL cSoundChannel::Play(void *Ptr, unsigned long Size, long VolumePercent, long Loop)
{
  DMUS_OBJECTDESC dmod;

  // Stop and free any playback
  Stop();

  // Get the object
  ZeroMemory(&dmod, sizeof(DMUS_OBJECTDESC));
  dmod.dwSize = sizeof(DMUS_OBJECTDESC);
  dmod.guidClass = CLSID_DirectMusicSegment;
  dmod.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_LOADED | DMUS_OBJ_MEMORY;
  dmod.llMemLength = (LONGLONG)Size;
  dmod.pbMemData = (LPBYTE)Ptr;
  if(FAILED(m_Sound->GetLoaderCOM()->GetObject(&dmod, IID_IDirectMusicSegment8, (LPVOID*)&m_pDMSegment)))
    return FALSE;

  // Download the segment's instruments to the audio path
  m_pDMSegment->Download(m_pDMPath);

  // Set looping data
  m_Loop = Loop;
  if(!m_Loop)
    m_pDMSegment->SetRepeats(DMUS_SEG_REPEAT_INFINITE);
  else
    m_pDMSegment->SetRepeats(m_Loop-1);

  // Set the volume
  SetVolume(VolumePercent);

  // Start playing
  if(FAILED(m_Sound->GetPerformanceCOM()->PlaySegmentEx(m_pDMSegment, NULL, NULL, DMUS_SEGF_SECONDARY, 0, NULL, m_pDMPath, m_pDMPath)))
    return FALSE;

  return TRUE;
}

BOOL cSoundChannel::Stop()
{
  // Stop any playback and unload sound
  if(m_pDMSegment) {
    m_Sound->GetPerformanceCOM()->Stop(m_pDMSegment, NULL, 0, 0);

    // Unload sound
    if(m_pDMPath)
      m_pDMSegment->Unload(m_pDMPath);
  }

  // Release the segment
  ReleaseCOM(m_pDMSegment);

  return TRUE;
}

long cSoundChannel::GetVolume()
{
  return m_Volume;
}

BOOL cSoundChannel::SetVolume(long Percent)
{
  long Volume;

  if(m_pDMPath == NULL)
    return FALSE;

  // Calculate a usable volume level
  Volume = ((Percent % 101) - 100) * 100;

  // Set the volume
  m_pDMPath->SetVolume(Volume, 0);

  m_Volume = Percent % 101;

  return TRUE;
}

BOOL cSoundChannel::IsPlaying()
{
  if(m_Sound == NULL)
    return FALSE;

  if(FAILED(m_Sound->GetPerformanceCOM()->IsPlaying(m_pDMSegment, NULL)))
    return FALSE;
  return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// cMusicChannel Functions
//
//////////////////////////////////////////////////////////////////////
cMusicChannel::cMusicChannel()
{
  m_Sound      = NULL;
  m_pDMSegment = NULL;
  m_Volume     = 0;
}

cMusicChannel::~cMusicChannel()
{
  Free();
}

IDirectMusicSegment8 *cMusicChannel::GetSegmentCOM()
{
  return m_pDMSegment;
}

BOOL cMusicChannel::Create(cSound *Sound)
{
  Free();

  // Make sure all objects there
  if((m_Sound = Sound) == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;
  if(m_Sound->GetLoaderCOM() == NULL)
    return FALSE;

  return TRUE;
}

BOOL cMusicChannel::Load(char *Filename)
{
  DMUS_OBJECTDESC dmod;

  Free();

  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;
  if(m_Sound->GetLoaderCOM() == NULL)
    return FALSE;

  // Get the object
  ZeroMemory(&dmod, sizeof(DMUS_OBJECTDESC));
  dmod.dwSize = sizeof(DMUS_OBJECTDESC);
  dmod.guidClass = CLSID_DirectMusicSegment;
  dmod.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
  mbstowcs(dmod.wszFileName, Filename, MAX_PATH);
  if(FAILED(m_Sound->GetLoaderCOM()->GetObject(&dmod, IID_IDirectMusicSegment8, (LPVOID*)&m_pDMSegment)))
    return FALSE;

  // Setup MIDI playing
  if(strstr(Filename, ".mid") != NULL) {
    if(FAILED(m_pDMSegment->SetParam(GUID_StandardMIDIFile, 0xFFFFFFFF, 0, 0, NULL)))
      return FALSE;
  }

  // Download the band
  if(FAILED(m_pDMSegment->Download(m_Sound->GetPerformanceCOM())))
    return FALSE;

  return TRUE;
}

BOOL cMusicChannel::Free()
{
  Stop();

  if(m_Sound != NULL) {
    // unload instrument data
    if(m_pDMSegment != NULL) {
      if(m_Sound->GetPerformanceCOM() != NULL) {
        if(FAILED(m_pDMSegment->Unload(m_Sound->GetPerformanceCOM())))
          return FALSE;
      }

      // free loader data
      if(m_Sound->GetLoaderCOM() != NULL) {
        if(FAILED(m_Sound->GetLoaderCOM()->ReleaseObjectByUnknown(m_pDMSegment)))
          return FALSE;
      }
    }
  }

  // release the segment
  ReleaseCOM(m_pDMSegment);
  
  return TRUE;
}

BOOL cMusicChannel::SetDLS(cDLS *DLS)
{
  if(DLS == NULL)
    return FALSE;
  if(DLS->GetCollectionCOM() == NULL)
    return FALSE;
  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;
  if(m_pDMSegment == NULL)
    return FALSE;

  // Connect to the collection
  if(FAILED(m_pDMSegment->SetParam(GUID_ConnectToDLSCollection, 0xFFFFFFFF, 0, 0, (void*)DLS->GetCollectionCOM())))
    return FALSE;

  // unload and then re-download new instruments
  if(FAILED(m_pDMSegment->Unload(m_Sound->GetPerformanceCOM())))
    return FALSE;
  if(FAILED(m_pDMSegment->Download(m_Sound->GetPerformanceCOM())))
    return FALSE;
  
  return TRUE;
}

BOOL cMusicChannel::Play(long VolumePercent, long Loop)
{
  Stop();

  // Return if not setup correctly
  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;
  if(m_pDMSegment == NULL)
    return FALSE;

  // Set the number of loops
  if(!Loop)
    m_pDMSegment->SetRepeats(DMUS_SEG_REPEAT_INFINITE);
  else
    m_pDMSegment->SetRepeats(Loop-1);

  // Set the playback volume
  SetVolume(VolumePercent);

  // Play on default audio path
  if(FAILED(m_Sound->GetPerformanceCOM()->PlaySegmentEx(
               m_pDMSegment, NULL, NULL, 
               0, 0, NULL, NULL, NULL)))
    return FALSE;

  return TRUE;
}

BOOL cMusicChannel::Stop()
{
  // Return if not setup correctly
  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;
  if(m_pDMSegment == NULL)
    return FALSE;

  // Stop playback 
  if(FAILED(m_Sound->GetPerformanceCOM()->Stop(m_pDMSegment, NULL, 0, 0)))
    return FALSE;

  return TRUE;
}

long cMusicChannel::GetVolume()
{
  return m_Volume;
}

BOOL cMusicChannel::SetVolume(long Percent)
{
  IDirectMusicAudioPath8 *pDMAudioPath;
  long Volume;

  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;

  if(FAILED(m_Sound->GetPerformanceCOM()->GetDefaultAudioPath(&pDMAudioPath)))
    return FALSE;

  // calculate a usable volume level
  if(!Percent)
    Volume = -9600;
  else
    Volume = (long)(-32.0 * (100.0 - (float)(Percent % 101)));

  if(FAILED(pDMAudioPath->SetVolume(Volume,0))) {
    pDMAudioPath->Release();
    return FALSE;
  }
  pDMAudioPath->Release();

  m_Volume = Percent % 101;

  return TRUE;
}

BOOL cMusicChannel::SetTempo(long Percent)
{
  float Tempo;

  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;

  // calculate tempo setting based on percentage
  Tempo = (float)Percent / 100.0f;

  // set master performance tempo
  if(FAILED(m_Sound->GetPerformanceCOM()->SetGlobalParam(GUID_PerfMasterTempo, (void*)&Tempo, sizeof(float))))
    return FALSE;

  return TRUE;
}

BOOL cMusicChannel::IsPlaying()
{
  // Return if not setup correctly
  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetPerformanceCOM() == NULL)
    return FALSE;
  if(m_pDMSegment == NULL)
    return FALSE;

  if(m_Sound->GetPerformanceCOM()->IsPlaying(m_pDMSegment, NULL) == S_OK)
    return TRUE;

  return FALSE;
}

cDLS::cDLS()
{
  m_Sound = NULL;
  m_pDMCollection = NULL;
}

cDLS::~cDLS()
{
  Free();
}

BOOL cDLS::Create(cSound *Sound)
{
  Free();

  if((m_Sound = Sound) == NULL)
    return FALSE;
  if(m_Sound->GetLoaderCOM() == NULL)
    return FALSE;

  return TRUE;
}

BOOL cDLS::Load(char *Filename)
{
  DMUS_OBJECTDESC dmod;

  Free();

  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetLoaderCOM() == NULL)
    return FALSE;

  ZeroMemory(&dmod, sizeof(DMUS_OBJECTDESC));
  dmod.dwSize = sizeof(DMUS_OBJECTDESC);
  dmod.guidClass = CLSID_DirectMusicCollection;

  if(Filename == NULL) {
    // Get the default collection
    dmod.guidObject = GUID_DefaultGMCollection;
    dmod.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_OBJECT;
  } else {
    // Get the collection object
    dmod.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
    mbstowcs(dmod.wszFileName, Filename, MAX_PATH);
  }

  if(FAILED(m_Sound->GetLoaderCOM()->GetObject(&dmod, IID_IDirectMusicCollection8, (LPVOID*)&m_pDMCollection)))
    return FALSE;

  return TRUE;
}

BOOL cDLS::Free()
{
  if(m_Sound == NULL)
    return FALSE;
  if(m_Sound->GetLoaderCOM() == NULL)
    return FALSE;

  if(m_pDMCollection != NULL) {
    if(FAILED(m_Sound->GetLoaderCOM()->ReleaseObjectByUnknown(m_pDMCollection)))
      return FALSE;
  }
  ReleaseCOM(m_pDMCollection);

  return TRUE;
}

IDirectMusicCollection8 *cDLS::GetCollectionCOM()
{
  return m_pDMCollection;
}

long cDLS::GetNumPatches()
{
  HRESULT hr;
  DWORD   dwPatch;
  long    i;

  hr = S_OK;
  for(i=0; hr == S_OK; i++) {
    hr = m_pDMCollection->EnumInstrument(i, &dwPatch, NULL, 0);
    if(hr != S_OK)
      break;
  }
  return i;
}

long cDLS::GetPatch(long Index)
{
  DWORD dwPatch;

  if(m_pDMCollection == NULL)
    return -1;

  if(FAILED(m_pDMCollection->EnumInstrument(Index, &dwPatch, NULL, 0)))
    return -1;

  return (long)dwPatch;
}

BOOL cDLS::Exists(long Patch)
{
  IDirectMusicInstrument8 *pDMInstrument;

  if(m_pDMCollection == NULL)
    return FALSE;

  if(FAILED(m_pDMCollection->GetInstrument(Patch, &pDMInstrument)))
    return FALSE;
  pDMInstrument->Release();

  return TRUE;
}

/**************************************************
WinMain.cpp
GameCore Component

Programming Role-Playing Games with DirectX
by Jim Adams (01 Jan 2002)
**************************************************/

#ifndef _CORE_SOUND_H_
#define _CORE_SOUND_H_

// Macro to release a COM object
#define ReleaseCOM(x) if(x) { x->Release(); x = NULL; }

// Macros to help create patches numbers
#define PATCH(m,l,p) ((m << 16) | (l << 8) | p)
#define PATCHMSB(x)  ((x >> 16) & 255)
#define PATCHLSB(x)  ((x >>  8) & 255)
#define PATCHNUM(x)  (x & 255)

// Forward class declarations
class cSound;
class cSoundChannel;
class cMusicChannel;
class cDLS;

class cSound
{
  protected:
    // Sound system related
    HWND                      m_hWnd;

    // Master volume level
    long                      m_Volume;

    // Playback format settings
    long                      m_Frequency;
    
    // Music and sound objects
    IDirectMusicPerformance8 *m_pDMPerformance;
    IDirectMusicLoader8      *m_pDMLoader;

  public:
    cSound();
    ~cSound();

    // Functions to retrieve COM interfaces
    IDirectMusicPerformance8 *GetPerformanceCOM();
    IDirectMusicLoader8      *GetLoaderCOM();

    // Init and shutdown functions
    BOOL Init(HWND hWnd, long Frequency = 22050);
    BOOL Shutdown();

    // Volume get/set
    long GetVolume();
    BOOL SetVolume(long Percent);
};

class cSoundChannel
{
  friend class cSound;

  protected:
    cSound                *m_Sound;

    IDirectMusicAudioPath *m_pDMPath;
    IDirectMusicSegment8  *m_pDMSegment;

    long                   m_Volume;
    long                   m_Loop;

  public:
    cSoundChannel();
    ~cSoundChannel();

    IDirectMusicAudioPath8 *GetAudioPathCOM();
    IDirectMusicSegment8 *GetSegmentCOM();
    
    BOOL Create(cSound *Sound);
    BOOL Free();
    
    BOOL Play(char *Filename, long VolumePercent = 100, long Loop = 1);
    BOOL Play(void *Ptr, unsigned long Size, long VolumePercent = 100, long Loop = 1);
    BOOL Stop();

    long GetVolume();
    BOOL SetVolume(long Percent);

    BOOL IsPlaying();
};

class cMusicChannel
{
  friend class cSound;

  protected:
    cSound               *m_Sound;
    IDirectMusicSegment8 *m_pDMSegment;
    long                  m_Volume;

  public:
    cMusicChannel();
    ~cMusicChannel();

    IDirectMusicSegment8 *GetSegmentCOM();
    
    BOOL Create(cSound *Sound);

    BOOL Load(char *Filename);
    BOOL Free();

    BOOL SetDLS(cDLS *DLS);
    
    BOOL Play(long VolumePercent = 100, long Loop = 1);
    BOOL Stop();

    long GetVolume();
    BOOL SetVolume(long Percent = 100);

    BOOL SetTempo(long Percent = 100);
    
    BOOL IsPlaying();
};

class cDLS
{
  protected:
    cSound                 *m_Sound;
    IDirectMusicCollection *m_pDMCollection;

  public:
    cDLS();
    ~cDLS();

    IDirectMusicCollection8 *GetCollectionCOM();

    BOOL Create(cSound *Sound);
    BOOL Load(char *Filename = NULL);
    BOOL Free();

    long GetNumPatches();
    long GetPatch(long Index);
    BOOL Exists(long Patch);
};

#pragma pack(1) 
typedef struct sWaveHeader
{
  char  RiffSig[4];
  long  WaveformChunkSize;
  char  WaveSig[4];
  char  FormatSig[4];
  long  FormatChunkSize;
  short FormatTag;
  short Channels;
  long  SampleRate;
  long  BytesPerSec;
  short BlockAlign;
  short BitsPerSample;
  char  DataSig[4];
  long  DataSize;
} sWaveHeader;
#pragma pack() 

#endif

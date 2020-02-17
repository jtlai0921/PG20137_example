/**************************************************
WinMain.cpp
Chapter 6 XFile/Skinned Mesh Demo

Programming Role-Playing Games with DirectX
by Jim Adams (01 Jan 2002)

Required libraries:
  WINMM.LIB, D3D8.LIB, D3DX8.LIB, and DXGUID.LIB
**************************************************/

// Macro to release COM objects
#define ReleaseCOM(x) if(x) { x->Release(); x = NULL; }

// Include files
#include <windows.h>
#include <stdio.h>
#include "d3d8.h"
#include "d3dx8.h"
#include "dxfile.h"
#include "rmxfguid.h"
#include "rmxftmpl.h"

// Window handles, class and caption text
HWND          g_hWnd;
HINSTANCE     g_hInst;
static char   g_szClass[]   = "XFileClass";
static char   g_szCaption[] = "XFile Demo by Jim Adams";

// The Direct3D and Device object
IDirect3D8       *g_pD3D       = NULL;
IDirect3DDevice8 *g_pD3DDevice = NULL;

// A mesh definition structure
typedef struct sMesh
{
  char               *m_Name;            // Name of mesh

  ID3DXMesh          *m_Mesh;            // Mesh object
  ID3DXSkinMesh      *m_SkinMesh;        // Skin mesh object

  DWORD               m_NumMaterials;    // # materials in mesh
  D3DMATERIAL8       *m_Materials;       // Array of materials
  IDirect3DTexture8 **m_Textures;        // Array of textures

  DWORD               m_NumBones;        // # of bones
  ID3DXBuffer        *m_BoneNames;       // Names of bones
  ID3DXBuffer        *m_BoneTransforms;  // Internal transformations

  D3DXMATRIX         *m_BoneMatrices;    // X file bone matrices

  sMesh              *m_Next;            // Next mesh in list

  sMesh()
  { 
    m_Name           = NULL;  // Clear all structure data
    m_Mesh           = NULL;
    m_SkinMesh       = NULL;
    m_NumMaterials   = 0;
    m_Materials      = NULL;
    m_Textures       = NULL;
    m_NumBones       = 0;
    m_BoneNames      = NULL;
    m_BoneTransforms = NULL;

    m_Next           = NULL;
  }

  ~sMesh()
  {
    // Free all used resources
    delete [] m_Name;
    ReleaseCOM(m_Mesh);
    ReleaseCOM(m_SkinMesh);
    delete [] m_Materials;
    if(m_Textures != NULL) {
      for(DWORD i=0;i<m_NumMaterials;i++) {
        ReleaseCOM(m_Textures[i]);
      }
      delete [] m_Textures;
    }
    ReleaseCOM(m_BoneNames);
    ReleaseCOM(m_BoneTransforms);

    delete m_Next;  // Delete next mesh in list
  }
} sMesh;

// Structure to contain frame information
typedef struct sFrame
{
  char       *m_Name;      // Frame's name

  sMesh      *m_Mesh;      // Linked list of meshes

  sFrame     *m_Sibling;   // Sibling frame
  sFrame     *m_Child;     // Child frame

  sFrame() 
  {
    // Clear all data
    m_Name = NULL;
    m_Mesh = NULL;
    m_Sibling = m_Child = NULL;
  }

  ~sFrame()
  {
    // Delete all used resources, including linked list of frames
    delete m_Name;
    delete m_Mesh;
    delete m_Child;
    delete m_Sibling;
  }
} sFrame;

// Parent frame for .X file
sFrame *g_pParentFrame = NULL;

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev,          \
                   LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg,              \
                           WPARAM wParam, LPARAM lParam);

BOOL DoInit();
BOOL DoShutdown();
BOOL DoFrame();
BOOL SetupMesh();

sFrame *LoadFile(char *Filename);
sFrame *ParseXFile(char *Filename);
void ParseXFileData(IDirectXFileData *pDataObj, sFrame *ParentFrame);
void DrawFrame(sFrame *Frame);

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev,          \
                   LPSTR szCmdLine, int nCmdShow)
{
  WNDCLASSEX wcex;
  MSG        Msg;

  g_hInst = hInst;

  // Create the window class here and register it
  wcex.cbSize        = sizeof(wcex);
  wcex.style         = CS_CLASSDC;
  wcex.lpfnWndProc   = WindowProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = hInst;
  wcex.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = NULL;
  wcex.lpszMenuName  = NULL;
  wcex.lpszClassName = g_szClass;
  wcex.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
  if(!RegisterClassEx(&wcex))
    return FALSE;

  // Create the Main Window
  g_hWnd = CreateWindow(g_szClass, g_szCaption,
        WS_CAPTION | WS_SYSMENU,
        0, 0, 400, 400,
        NULL, NULL,
        hInst, NULL );
  if(!g_hWnd)
    return FALSE;
  ShowWindow(g_hWnd, SW_NORMAL);
  UpdateWindow(g_hWnd);

  // Run init function and return on error
  if(DoInit() == FALSE)
    return FALSE;

  // Start message pump, waiting for signal to quit
  ZeroMemory(&Msg, sizeof(MSG));
  while(Msg.message != WM_QUIT) {
    if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
    if(DoFrame() == FALSE)
      break;
  }

  // Run shutdown function
  DoShutdown();
  
  UnregisterClass(g_szClass, hInst);

  return Msg.wParam;
}

long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg,              \
                           WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;

  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL DoInit()
{
  D3DPRESENT_PARAMETERS d3dpp;
  D3DDISPLAYMODE        d3ddm;
  D3DXMATRIX matProj, matView;

  // Do a windowed mode initialization of Direct3D
  if((g_pD3D = Direct3DCreate8(D3D_SDK_VERSION)) == NULL)
    return FALSE;
  if(FAILED(g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
    return FALSE;
  ZeroMemory(&d3dpp, sizeof(d3dpp));
  d3dpp.Windowed = TRUE;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.BackBufferFormat = d3ddm.Format;
  d3dpp.EnableAutoDepthStencil = TRUE;
  d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
  if(FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                  &d3dpp, &g_pD3DDevice)))
    return FALSE;

  // Set the rendering states
  g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
  g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

  // Create and set the projection matrix
  D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4.0f, 1.33333f, 1.0f, 1000.0f);
  g_pD3DDevice->SetTransform(D3DTS_PROJECTION, &matProj);

  // Create and set the view matrix
  D3DXMatrixLookAtLH(&matView,                                \
                     &D3DXVECTOR3(0.0f, 50.0f, -150.0f),     \
                     &D3DXVECTOR3(0.0f, 50.0f, 0.0f),          \
                     &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Load a skinned mesh from an .X file
  g_pParentFrame = LoadFile("warrior.x");

  return TRUE;
}

BOOL DoShutdown()
{
  // Release frames and meshes
  if(g_pParentFrame != NULL)
    delete g_pParentFrame;

  // Release device and 3D objects
  if(g_pD3DDevice != NULL)
    g_pD3DDevice->Release();

  if(g_pD3D != NULL)
    g_pD3D->Release();

  return TRUE;
}

BOOL DoFrame()
{
  D3DXMATRIX matWorld;

  // Clear device backbuffer
  g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                      D3DCOLOR_RGBA(0,64,128,255), 1.0f, 0);

  // Begin scene
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Set world transformation to rotate on Y-axis
    D3DXMatrixRotationY(&matWorld, (float)timeGetTime() / 1000.0f);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Draw frames
    DrawFrame(g_pParentFrame);

    // Release texture
    g_pD3DDevice->SetTexture(0, NULL);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Display the scene
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);

  return TRUE;
}


sFrame *LoadFile(char *Filename)
{
  return ParseXFile(Filename);
}

sFrame *ParseXFile(char *Filename)
{
  IDirectXFile           *pDXFile = NULL;
  IDirectXFileEnumObject *pDXEnum = NULL;
  IDirectXFileData       *pDXData = NULL;
  sFrame                 *Frame;

  // Create the file object
  if(FAILED(DirectXFileCreate(&pDXFile)))
    return FALSE;

  // Register the templates
  if(FAILED(pDXFile->RegisterTemplates((LPVOID)D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES))) {
    pDXFile->Release();
    return FALSE;
  }

  // Create an enumeration object
  if(FAILED(pDXFile->CreateEnumObject((LPVOID)Filename, DXFILELOAD_FROMFILE, &pDXEnum))) {
    pDXFile->Release();
    return FALSE;
  }

  // Allocate a frame that becomes root
  Frame = new sFrame();

  // Loop through all objects looking for the frames and meshes
  while(SUCCEEDED(pDXEnum->GetNextDataObject(&pDXData))) {
    ParseXFileData(pDXData, Frame);
    ReleaseCOM(pDXData);
  }

  // Release used COM objects
  ReleaseCOM(pDXEnum);
  ReleaseCOM(pDXFile);

  // Return root frame
  return Frame;
}

void ParseXFileData(IDirectXFileData *pDataObj, sFrame *ParentFrame)
{
  IDirectXFileObject *pSubObj  = NULL;
  IDirectXFileData   *pSubData = NULL;
  IDirectXFileDataReference *pDataRef = NULL;
  const GUID *Type = NULL;
  char       *Name = NULL;
  DWORD       Size;

  sFrame     *Frame = NULL;
  sFrame     *SubFrame = NULL;

  sMesh         *Mesh = NULL;
  ID3DXBuffer   *MaterialBuffer = NULL;
  D3DXMATERIAL  *Materials = NULL;
  ID3DXBuffer   *Adjacency = NULL;
  DWORD         *AdjacencyIn = NULL;
  DWORD         *AdjacencyOut = NULL;

  DWORD i;

  // Get the template type
  if(FAILED(pDataObj->GetType(&Type)))
    return;

  // Get the template name (if any)
  if(FAILED(pDataObj->GetName(NULL, &Size)))
    return;
  if(Size) {
    if((Name = new char[Size]) != NULL)
      pDataObj->GetName(Name, &Size);
  }

  // Give template a default name if none found
  if(Name == NULL) {
    if((Name = new char[9]) == NULL)
      return;
    strcpy(Name, "Template");
  }

  // Set sub frame
  SubFrame = ParentFrame;

  // Process the templates

  // Frame
  if(*Type == TID_D3DRMFrame) {
    // Create a new frame structure
    Frame = new sFrame();

    // Store the name
    Frame->m_Name = Name;
    Name = NULL;

    // Add to parent frame
    Frame->m_Sibling = ParentFrame->m_Child;
    ParentFrame->m_Child = Frame;

    // Set sub frame parent
    SubFrame = Frame;
  }

  // Load a mesh
  if(*Type == TID_D3DRMMesh) {
    // Create a new mesh structure
    Mesh = new sMesh();

    // Store the name
    Mesh->m_Name = Name;
    Name = NULL;

    // Load mesh data (as a skinned mesh)
    if(FAILED(D3DXLoadSkinMeshFromXof(pDataObj, 0,
                g_pD3DDevice,
                &Adjacency,
                &MaterialBuffer, &Mesh->m_NumMaterials, 
                &Mesh->m_BoneNames, &Mesh->m_BoneTransforms,
                &Mesh->m_SkinMesh))) {
      delete Mesh;
      return;
    }

    // Convert to regular mesh if no bones
    if(!(Mesh->m_NumBones = Mesh->m_SkinMesh->GetNumBones())) {
    
      // Convert to a regular mesh
      Mesh->m_SkinMesh->GetOriginalMesh(&Mesh->m_Mesh);
      ReleaseCOM(Mesh->m_SkinMesh);

    } else {

      // Get a pointer to bone matrices
      Mesh->m_BoneMatrices = (D3DXMATRIX*)Mesh->m_BoneTransforms->GetBufferPointer();

      // Allocate buffers for adjacency info
      AdjacencyIn  = (DWORD*)Adjacency->GetBufferPointer();
      AdjacencyOut = new DWORD[Mesh->m_SkinMesh->GetNumFaces() * 3];

      // Generate the skin mesh object
      if(FAILED(Mesh->m_SkinMesh->GenerateSkinnedMesh(
                  0, 0.0f, 
                  AdjacencyIn, AdjacencyOut, 
                  NULL, NULL,
                  &Mesh->m_Mesh))) {
        // Convert to a regular mesh if error
        Mesh->m_SkinMesh->GetOriginalMesh(&Mesh->m_Mesh);
        ReleaseCOM(Mesh->m_SkinMesh);
        Mesh->m_NumBones = 0;
      }

      delete [] AdjacencyOut;
      ReleaseCOM(Adjacency);
    }

    // Load materials or create a default one if none
    if(!Mesh->m_NumMaterials) {
      // Create a default one
      Mesh->m_Materials = new D3DMATERIAL8[1];
      Mesh->m_Textures  = new LPDIRECT3DTEXTURE8[1];

      ZeroMemory(Mesh->m_Materials, sizeof(D3DMATERIAL8));
      Mesh->m_Materials[0].Diffuse.r = 1.0f;
      Mesh->m_Materials[0].Diffuse.g = 1.0f;
      Mesh->m_Materials[0].Diffuse.b = 1.0f;
      Mesh->m_Materials[0].Diffuse.a = 1.0f;
      Mesh->m_Materials[0].Ambient   = Mesh->m_Materials[0].Diffuse;
      Mesh->m_Materials[0].Specular  = Mesh->m_Materials[0].Diffuse;
      Mesh->m_Textures[0] = NULL;

      Mesh->m_NumMaterials = 1;
    } else {
      // Load the materials
      Materials = (D3DXMATERIAL*)MaterialBuffer->GetBufferPointer();
      Mesh->m_Materials = new D3DMATERIAL8[Mesh->m_NumMaterials];
      Mesh->m_Textures  = new LPDIRECT3DTEXTURE8[Mesh->m_NumMaterials];

      for(i=0;i<Mesh->m_NumMaterials;i++) {
        Mesh->m_Materials[i] = Materials[i].MatD3D;
        Mesh->m_Materials[i].Ambient = Mesh->m_Materials[i].Diffuse;
     
        // Build a texture path and load it
        if(FAILED(D3DXCreateTextureFromFile(g_pD3DDevice,
                                            Materials[i].pTextureFilename,
                                            &Mesh->m_Textures[i]))) {
          Mesh->m_Textures[i] = NULL;
        }
      }
    }
    ReleaseCOM(MaterialBuffer);

    // Link in mesh
    Mesh->m_Next = ParentFrame->m_Mesh;
    ParentFrame->m_Mesh = Mesh;
  }

  // Skip animation sets and animations
  if(*Type == TID_D3DRMAnimationSet || *Type == TID_D3DRMAnimation || *Type == TID_D3DRMAnimationKey) {
    delete [] Name;
    return;
  }

  // Release name buffer
  delete [] Name;

  // Scan for embedded templates
  while(SUCCEEDED(pDataObj->GetNextObject(&pSubObj))) {

    // Process embedded references
    if(SUCCEEDED(pSubObj->QueryInterface(IID_IDirectXFileDataReference, (void**)&pDataRef))) {
      if(SUCCEEDED(pDataRef->Resolve(&pSubData))) {
        ParseXFileData(pSubData, SubFrame);
        ReleaseCOM(pSubData);
      }
      ReleaseCOM(pDataRef);
    }

    // Process non-referenced embedded templates
    if(SUCCEEDED(pSubObj->QueryInterface(IID_IDirectXFileData, (void**)&pSubData))) {
      ParseXFileData(pSubData, SubFrame);
      ReleaseCOM(pSubData);
    }
    ReleaseCOM(pSubObj);
  }

  return;
}

void DrawFrame(sFrame *Frame)
{
  sMesh *Mesh;
  D3DXMATRIX *Matrices = NULL;
  DWORD i;

  // Return if no frame
  if(Frame == NULL)
   return;

  // Draw meshes if any in frame
  if((Mesh = Frame->m_Mesh) != NULL) {

   // Generate mesh from skinned mesh to draw with
   if(Mesh->m_SkinMesh != NULL) {
     // Allocate an array of matrices to orient bones
     Matrices = new D3DXMATRIX[Mesh->m_NumBones];

     // Set all bones orientation to identity
     for(i=0;i<Mesh->m_NumBones;i++)
       D3DXMatrixIdentity(&Matrices[i]);

     // Update skinned mesh
     Mesh->m_SkinMesh->UpdateSkinnedMesh(Matrices, NULL, Mesh->m_Mesh);
   }

   // Render the mesh
   for(i=0;i<Mesh->m_NumMaterials;i++) {
     g_pD3DDevice->SetMaterial(&Mesh->m_Materials[i]);
     g_pD3DDevice->SetTexture(0, Mesh->m_Textures[i]);
     Mesh->m_Mesh->DrawSubset(i);
   }

   // Free array of matrices
   delete [] Matrices;
   Matrices = NULL;

   // Go to next mesh
   Mesh = Mesh->m_Next;
  }

  // Draw child frames
  DrawFrame(Frame->m_Child);

  // Draw sibling frames
  DrawFrame(Frame->m_Sibling);
}

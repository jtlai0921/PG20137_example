/**************************************************
WinMain.cpp
Chapter 6 Enum Demo

Programming Role-Playing Games with DirectX
by Jim Adams (01 Jan 2002)

Required libraries:
  D3D8.LIB
**************************************************/

#include <windows.h>
#include <stdio.h>

#include <d3d8.h>
#include "resource.h"

// Application variables ////////////////////////////////////////////
HWND g_hWnd;                    // Window handle
char g_szClass[] = "EnumDemo";  // Class name

IDirect3D8       *g_pD3D;        // Direct3D component
IDirect3DDevice8 *g_pD3DDevice;  // Direct3D Device component

// Application prototypes ///////////////////////////////////////////
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL InitD3D();
void ReleaseD3D();
void EnumAdapters();
void EnumModes(long AdapterNum);

// Application //////////////////////////////////////////////////////
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow)
{
  WNDCLASS wc;
  MSG      Msg;

  // Register window class
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = WindowProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = DLGWINDOWEXTRA;
  wc.hInstance     = hInst;
  wc.hIcon         = LoadIcon(hInst, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = g_szClass;
  RegisterClass(&wc);

  // Create the dialog box window and show it
  g_hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_ENUM), 0, NULL);

  // Initialize Direct3D and enumerate adapters
  if(InitD3D() == TRUE)
    EnumAdapters();
  else {
    MessageBox(NULL, "Error initializing Direct3D.",          \
               "ERROR", MB_OK | MB_ICONEXCLAMATION);
    return FALSE;
  }

  UpdateWindow(g_hWnd);
  ShowWindow(g_hWnd, nCmdShow);

  // Message loop
  while(GetMessage(&Msg, NULL, 0, 0)) {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

  // Clean up
  UnregisterClass(g_szClass, hInst);

  return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int Num;

  switch(uMsg) {
    case WM_COMMAND:
      // If user clicked on an adapter, enumerate modes
      if(LOWORD(wParam)==IDC_ADAPTERS && HIWORD(wParam)==LBN_SELCHANGE) {

        // Get selection number
        Num = SendMessage(GetDlgItem(hWnd, IDC_ADAPTERS), LB_GETCURSEL, 0, 0);

        // Enumerate modes
        EnumModes(Num);
      }
      break;

    case WM_DESTROY:
      ReleaseD3D();
      PostQuitMessage(0);
      break;

    default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

BOOL InitD3D()
{
  // Create Direct3D component
  if((g_pD3D = Direct3DCreate8(D3D_SDK_VERSION)) == NULL)
    return FALSE;

  return TRUE;
}

void ReleaseD3D()
{
  // Release Direct3D component
  if(g_pD3D != NULL)
    g_pD3D->Release();
  g_pD3D = NULL;
}

void EnumAdapters()
{
  D3DADAPTER_IDENTIFIER8 d3dai;
  long Num, i;
  
  // Clear the list box
  SendMessage(GetDlgItem(g_hWnd, IDC_ADAPTERS), LB_RESETCONTENT, 0, 0);

  // Get the number of adapters
  Num = g_pD3D->GetAdapterCount();

  // Enumerator adapters and add them to list box
  for(i=0;i<Num;i++) {
    // Get the adapter description
    g_pD3D->GetAdapterIdentifier(i, D3DENUM_NO_WHQL_LEVEL, &d3dai);

    // Add adapter description to list box
    SendMessage(GetDlgItem(g_hWnd, IDC_ADAPTERS), LB_INSERTSTRING, i, (LPARAM)d3dai.Description);
  }
}

void EnumModes(long AdapterNum)
{
  D3DDISPLAYMODE d3ddm;
  char Text[256];
  long Num, i;

  // Clear the list box
  SendMessage(GetDlgItem(g_hWnd, IDC_MODES), LB_RESETCONTENT, 0, 0);

  // Get the number of adapters
  Num = g_pD3D->GetAdapterModeCount(AdapterNum);

  // Enumerator modes and add them to display
  for(i=0;i<Num;i++) {
    // Enumerate the mode info
    g_pD3D->EnumAdapterModes(AdapterNum, i, &d3ddm);

    // Build a text string of mode format
    sprintf(Text, "%lu x %lu x ", d3ddm.Width, d3ddm.Height);
    switch(d3ddm.Format) {

      // Add 8-bit identifier
      case D3DFMT_A8:
      case D3DFMT_R3G3B2:
      case D3DFMT_P8:
        strcat(Text, "8");
        break;

      // Add 16-bit identifier
      case D3DFMT_R5G6B5:
      case D3DFMT_A1R5G5B5:
      case D3DFMT_X1R5G5B5:
      case D3DFMT_A4R4G4B4:
      case D3DFMT_A8R3G3B2:
      case D3DFMT_X4R4G4B4:
      case D3DFMT_A8P8:
        strcat(Text, "16");
        break;

      // Add 24-bit identifier
      case D3DFMT_R8G8B8:
        strcat(Text, "24");
        break;

      // Add 32-bit identifier
      case D3DFMT_A8R8G8B8:
      case D3DFMT_X8R8G8B8:
        strcat(Text, "32");
        break;
    }

    // Add mode description to list box
    SendMessage(GetDlgItem(g_hWnd, IDC_MODES), LB_INSERTSTRING, i, (LPARAM)Text);
  }
}

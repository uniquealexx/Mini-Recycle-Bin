#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#define ID_TRAY_ICON 1
#define ID_EXIT_MENU 2
#define ID_CLEAR_MENU 3
#define ID_CHANGE_THEME_MENU 4
#define ID_TIMER 5
#define TIMER_INTERVAL 1000

HINSTANCE hInst;
NOTIFYICONDATA nid;
HICON hIconEmpty;
HICON hIconFull;
HICON hIconRecycler;
HICON hIconRecyclerFull;
BOOL useDefaultIcons = TRUE;

BOOL IsRecycleBinEmpty() 
{
    SHQUERYRBINFO rbi = { sizeof(SHQUERYRBINFO) };
    if (SHQueryRecycleBin(NULL, &rbi) == S_OK) 
        return (rbi.i64NumItems == 0);

    return TRUE;
}

void UpdateTrayIcon() 
{
    if (useDefaultIcons) 
    {
        if (IsRecycleBinEmpty())
            nid.hIcon = hIconRecycler;
        else
            nid.hIcon = hIconRecyclerFull;
    }
    else 
    {
        if (IsRecycleBinEmpty())
            nid.hIcon = hIconEmpty;
        else
            nid.hIcon = hIconFull; 
    }

    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    switch (uMsg) 
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_EXIT_MENU) 
        {
            KillTimer(hwnd, ID_TIMER);
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
        }
        else if (LOWORD(wParam) == ID_CLEAR_MENU) 
        {
            SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
            UpdateTrayIcon();
        }
        else if (LOWORD(wParam) == ID_CHANGE_THEME_MENU) 
        {
            useDefaultIcons = !useDefaultIcons;
            UpdateTrayIcon();
        }
        break;
    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    case WM_USER + 1:
        if (lParam == WM_LBUTTONDBLCLK) 
        {
            ShellExecute(NULL, L"open", L"shell:RecycleBinFolder", NULL, NULL, SW_SHOWNORMAL);
        }
        else if (lParam == WM_RBUTTONUP) 
        {
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_CLEAR_MENU, L"Clear");
            AppendMenu(hMenu, MF_STRING, ID_CHANGE_THEME_MENU, L"Change theme");
            AppendMenu(hMenu, MF_STRING, ID_EXIT_MENU, L"Exit");
            SetForegroundWindow(hwnd);
            POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
        break;
    case WM_TIMER:
        if (wParam == ID_TIMER) 
        {
            UpdateTrayIcon();
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
    hInst = hInstance;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MiniBin";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, L"Mini Bin", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;

    SHSTOCKICONINFO sii;
    sii.cbSize = sizeof(SHSTOCKICONINFO);

    hIconEmpty = (HICON)LoadImage(NULL, L"Icons/EmptyTrash.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);
    hIconFull = (HICON)LoadImage(NULL, L"Icons/FullTrash.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);

    if (SUCCEEDED(SHGetStockIconInfo(SIID_RECYCLER, SHGSI_ICON | SHGSI_LARGEICON, &sii))) 
        hIconRecycler = sii.hIcon;
    if (SUCCEEDED(SHGetStockIconInfo(SIID_RECYCLERFULL, SHGSI_ICON | SHGSI_LARGEICON, &sii))) 
        hIconRecyclerFull = sii.hIcon;

    if (!hIconEmpty || !hIconFull || !hIconRecycler || !hIconRecyclerFull) 
    {
        MessageBox(NULL, L"Failed to load icon!", L"Error", MB_ICONERROR);
        return 1;
    }

    lstrcpy(nid.szTip, L"Mini Bin Tools");
    UpdateTrayIcon();
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetTimer(hwnd, ID_TIMER, TIMER_INTERVAL, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(hwnd, ID_TIMER);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    return (int)msg.wParam;
}

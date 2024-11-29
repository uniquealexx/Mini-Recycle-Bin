#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <iostream>

enum class EActions
{
    Clear = 1,
    ChangeTheme,
    Exit
};

#define WM_CONTEXT_NOTIFY (WM_USER + 1)
#define WM_ICON_CHANGE_NOTIFY (WM_USER + 2)

HINSTANCE hInst;
NOTIFYICONDATA NotifyData;
HICON hIconEmpty;
HICON hIconFull;
HICON hIconRecycler;
HICON hIconRecyclerFull;
BOOL bUseDefaultIcons = TRUE;

BOOL IsRecycleBinEmpty()
{
    SHQUERYRBINFO RbiInfo = { sizeof(SHQUERYRBINFO) };
    if (SHQueryRecycleBin(NULL, &RbiInfo) == S_OK)
        return (RbiInfo.i64NumItems == 0);

    return TRUE;
}

void UpdateTrayIcon()
{
    if (IsRecycleBinEmpty())
        NotifyData.hIcon = bUseDefaultIcons ? hIconRecycler : hIconEmpty;
    else
        NotifyData.hIcon = bUseDefaultIcons ? hIconRecyclerFull : hIconFull;

    Shell_NotifyIcon(NIM_MODIFY, &NotifyData);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == (int)EActions::Clear)
        {
            SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
            UpdateTrayIcon();
        }
        else if (LOWORD(wParam) == (int)EActions::ChangeTheme)
        {
            bUseDefaultIcons = !bUseDefaultIcons;
            UpdateTrayIcon();
        }
        else if (LOWORD(wParam) == (int)EActions::Exit)
        {
            Shell_NotifyIcon(NIM_DELETE, &NotifyData);
            PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &NotifyData);
        PostQuitMessage(0);
        break;
    case WM_CONTEXT_NOTIFY:
        if (lParam == WM_LBUTTONDBLCLK)
            ShellExecute(NULL, L"open", L"shell:RecycleBinFolder", NULL, NULL, SW_SHOWNORMAL);
        else if (lParam == WM_RBUTTONUP)
        {
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, (int)EActions::Clear, L"Clear");
            AppendMenu(hMenu, MF_STRING, (int)EActions::ChangeTheme, L"Change theme");
            AppendMenu(hMenu, MF_STRING, (int)EActions::Exit, L"Exit");
            SetForegroundWindow(hWnd);
            POINT Point;
            GetCursorPos(&Point);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN, Point.x, Point.y, 0, hWnd, NULL);
            PostMessage(hWnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
        break;
    case WM_ICON_CHANGE_NOTIFY:
        UpdateTrayIcon();
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;

    WNDCLASS WndClass = { 0 };
    WndClass.lpfnWndProc = WindowProc;
    WndClass.hInstance = hInstance;
    WndClass.lpszClassName = L"MiniBin";
    RegisterClass(&WndClass);

    HWND hWnd = CreateWindow(WndClass.lpszClassName, L"Mini Bin", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    NotifyData.cbSize = sizeof(NOTIFYICONDATA);
    NotifyData.hWnd = hWnd;
    NotifyData.uID = NULL;
    NotifyData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    NotifyData.uCallbackMessage = WM_CONTEXT_NOTIFY;

    SHSTOCKICONINFO shIconInfo;
    shIconInfo.cbSize = sizeof(SHSTOCKICONINFO);

    hIconEmpty = (HICON)LoadImage(NULL, L"Icons/EmptyTrash.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);
    hIconFull = (HICON)LoadImage(NULL, L"Icons/FullTrash.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);

    if (SUCCEEDED(SHGetStockIconInfo(SIID_RECYCLER, SHGSI_ICON | SHGSI_LARGEICON, &shIconInfo)))
        hIconRecycler = shIconInfo.hIcon;
    if (SUCCEEDED(SHGetStockIconInfo(SIID_RECYCLERFULL, SHGSI_ICON | SHGSI_LARGEICON, &shIconInfo)))
        hIconRecyclerFull = shIconInfo.hIcon;

    if (!hIconEmpty || !hIconFull || !hIconRecycler || !hIconRecyclerFull)
    {
        MessageBox(NULL, L"Failed to load icon!", L"Error", MB_ICONERROR);
        return 1;
    }

    lstrcpy(NotifyData.szTip, L"Mini Bin Tools");
    UpdateTrayIcon();
    Shell_NotifyIcon(NIM_ADD, &NotifyData);

    SHChangeNotifyEntry shCNE;
    shCNE.pidl = NULL;
    shCNE.fRecursive = FALSE;

    ULONG hNotify = SHChangeNotifyRegister(hWnd, SHCNRF_ShellLevel, SHCNE_UPDATEITEM | SHCNE_UPDATEIMAGE | SHCNE_RMDIR | SHCNE_MKDIR | SHCNE_DELETE, WM_ICON_CHANGE_NOTIFY, 1, &shCNE);
    if (!hNotify)
    {
        MessageBox(NULL, L"Failed to register for notifications!", L"Error", MB_ICONERROR);
        return 1;
    }

    MSG Message;
    while (GetMessage(&Message, NULL, 0, 0))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    if (hNotify)
        SHChangeNotifyDeregister(hNotify);

    Shell_NotifyIcon(NIM_DELETE, &NotifyData);
    return (int)Message.wParam;
}
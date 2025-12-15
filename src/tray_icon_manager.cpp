#include "tray_icon_manager.h"
#include "common.h"
#include "logger.h"
#include <shellapi.h>
#include <tchar.h>
#include <strsafe.h>

// Global variables for tray icon
NOTIFYICONDATA g_notifyIconData = {0};
bool g_trayIconAdded = false;
HMENU g_trayMenu = NULL;

// Add system tray icon
void AddTrayIcon()
{
    // If tray icon is already added, remove it first
    if (g_trayIconAdded)
    {
        RemoveTrayIcon();
    }
    
    // Set tray icon data
    g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    g_notifyIconData.hWnd = g_hMainWindow;
    g_notifyIconData.uID = 1;
    g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_notifyIconData.uCallbackMessage = WM_TRAYICON;
    // Load custom icon from file
    g_notifyIconData.hIcon = (HICON)LoadImageW(
        NULL, 
        L"app_icon.ico", 
        IMAGE_ICON, 
        0, 
        0, 
        LR_LOADFROMFILE | LR_DEFAULTSIZE
    );
    
    // If loading custom icon fails, use resource icon as fallback
    if (!g_notifyIconData.hIcon) {
#ifdef IDI_APP_ICON
        g_notifyIconData.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
#else
        g_notifyIconData.hIcon = LoadIcon(NULL, IDI_APPLICATION);
#endif
        LogToFile("AddTrayIcon: Failed to load custom icon, using resource icon");
    }
    
    // Use memcpy to copy string to avoid type issues
    const wchar_t* tipText = L"Quick Launcher";
    memcpy(g_notifyIconData.szTip, tipText, (wcslen(tipText) + 1) * sizeof(wchar_t));
    
    // Add tray icon
    if (Shell_NotifyIcon(NIM_ADD, &g_notifyIconData))
    {
        g_trayIconAdded = true;
        LogToFile("AddTrayIcon: Successfully added system tray icon");
    }
    else
    {
        LogToFile("AddTrayIcon: Failed to add system tray icon");
    }
}

// Remove system tray icon
void RemoveTrayIcon()
{
    if (g_trayIconAdded)
    {
        if (Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData))
        {
            g_trayIconAdded = false;
            LogToFile("RemoveTrayIcon: Successfully removed system tray icon");
        }
        else
        {
            LogToFile("RemoveTrayIcon: Failed to remove system tray icon");
        }
    }
}

// Create tray context menu
void CreateTrayMenu()
{
    // If menu already exists, destroy it first
    if (g_trayMenu)
    {
        DestroyMenu(g_trayMenu);
    }
    
    // Create popup menu
    g_trayMenu = CreatePopupMenu();
    if (g_trayMenu)
    {
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_SHOW, L"显示窗口");
        AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_EXIT, L"退出");
        LogToFile("CreateTrayMenu: Successfully created tray context menu");
    }
    else
    {
        LogToFile("CreateTrayMenu: Failed to create tray context menu");
    }
}

// Handle tray messages
void HandleTrayMessage(LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
        // Double click left button to show window
        ShowLauncherWindow();
        LogToFile("HandleTrayMessage: Double clicked tray icon, showing window");
        break;
        
    case WM_RBUTTONDOWN:
        // Right click to show menu
        if (g_trayMenu)
        {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(g_hMainWindow);
            
            // Show menu and get user selection
            UINT cmd = TrackPopupMenu(g_trayMenu, 
                                     TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                     pt.x, pt.y, 0, g_hMainWindow, NULL);
            
            // Handle menu selection
            switch (cmd)
            {
            case ID_TRAY_SHOW:
                ShowLauncherWindow();
                LogToFile("HandleTrayMessage: User selected Show Window");
                break;
                
            case ID_TRAY_EXIT:
                PostMessage(g_hMainWindow, WM_CLOSE, 0, 0);
                LogToFile("HandleTrayMessage: User selected Exit");
                break;
            }
        }
        break;
    }
}

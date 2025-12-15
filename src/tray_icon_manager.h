#ifndef TRAY_ICON_MANAGER_H
#define TRAY_ICON_MANAGER_H

#include <windows.h>

// Global variables declaration
extern bool g_trayIconAdded;
extern HMENU g_trayMenu;

// Adds the tray icon to the system tray
void AddTrayIcon();

// Removes the tray icon from the system tray
void RemoveTrayIcon();

// Creates the tray icon context menu
void CreateTrayMenu();

// Handles tray icon messages
void HandleTrayMessage(LPARAM lParam);

#endif // TRAY_ICON_MANAGER_H

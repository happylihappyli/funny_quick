#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "common.h"

// Command processing functions
void ProcessCommand(const WCHAR* command);
void SearchAndDisplayResults(const WCHAR* query);
void ExecuteSelectedItem(INT_PTR index);
void ExecuteFileModeItem(INT_PTR index);

// Shortcut management
int ImportDesktopShortcuts();
void AddDesktopShortcuts();
void InitializeCommonShortcuts();

// Helper functions (exposed if needed by other modules, otherwise internal)
void LogListViewContents();
void UpdateWebViewForSearch(const std::vector<std::wstring>& webViewHints);
void ProcessSearchQuery(const WCHAR* query, std::vector<std::wstring>& webViewHints);

#endif // COMMAND_PROCESSOR_H

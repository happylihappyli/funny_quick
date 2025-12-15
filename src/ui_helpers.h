#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include <windows.h>
#include <string>

// Font related
void CreateUIFont();
void ApplyFontToControl(HWND hWnd);

// Layout related
void LayoutControls(int windowWidth, int windowHeight);
void UpdateListViewColumns();
void AddHintRowToListView(const WCHAR* hintText);
void AddMultiLineHintsToListView(const WCHAR* hints[], int hintCount);
int GetHintRowCount();
INT_PTR GetFirstActualItemIndex();

// HTML related
std::wstring ReadHtmlTemplate(const std::wstring& filePath);
void ReplaceStringInPlace(std::wstring& str, const std::wstring& from, const std::wstring& to);

#endif // UI_HELPERS_H

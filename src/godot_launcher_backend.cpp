#include "godot_launcher_backend.h"

#include <algorithm>
#include <functional>
#include <set>

#include <shlobj.h>
#include <knownfolders.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <shellapi.h>

#include "logger.h"

static std::wstring ToLower(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), towlower);
    return s;
}

static bool ContainsI(const std::wstring& haystack, const std::wstring& needle)
{
    if (needle.empty())
    {
        return true;
    }
    std::wstring h = ToLower(haystack);
    std::wstring n = ToLower(needle);
    return h.find(n) != std::wstring::npos;
}

static std::wstring GetModuleDir()
{
    WCHAR modulePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, modulePath, MAX_PATH);
    std::wstring moduleDir = modulePath;
    size_t lastBackslash = moduleDir.find_last_of(L"\\");
    if (lastBackslash != std::wstring::npos)
    {
        moduleDir = moduleDir.substr(0, lastBackslash);
    }
    return moduleDir;
}

static std::wstring GetParentDir(const std::wstring& dir)
{
    std::wstring parentDir = dir;
    size_t parentBackslash = parentDir.find_last_of(L"\\");
    if (parentBackslash != std::wstring::npos)
    {
        parentDir = parentDir.substr(0, parentBackslash);
    }
    return parentDir;
}

static std::wstring GetShortcutsFilePath()
{
    std::wstring moduleDir = GetModuleDir();
    std::wstring parentDir = GetParentDir(moduleDir);

    std::wstring rootFile = parentDir + L"\\data\\shortcuts.txt";
    std::wstring binFile = moduleDir + L"\\data\\shortcuts.txt";
    if (GetFileAttributesW(rootFile.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        return rootFile;
    }
    return binFile;
}

static bool EnsureShortcutsFileDir(std::wstring& outFilePath)
{
    std::wstring moduleDir = GetModuleDir();
    std::wstring parentDir = GetParentDir(moduleDir);
    std::wstring rootDataDir = parentDir + L"\\data";
    std::wstring rootFile = rootDataDir + L"\\shortcuts.txt";

    FILE* test = _wfopen(rootFile.c_str(), L"a, ccs=UTF-8");
    if (test)
    {
        fclose(test);
        outFilePath = rootFile;
        return true;
    }

    CreateDirectoryW(rootDataDir.c_str(), NULL);
    test = _wfopen(rootFile.c_str(), L"a, ccs=UTF-8");
    if (test)
    {
        fclose(test);
        outFilePath = rootFile;
        return true;
    }

    std::wstring binDataDir = moduleDir + L"\\data";
    std::wstring binFile = binDataDir + L"\\shortcuts.txt";
    CreateDirectoryW(binDataDir.c_str(), NULL);
    test = _wfopen(binFile.c_str(), L"a, ccs=UTF-8");
    if (test)
    {
        fclose(test);
        outFilePath = binFile;
        return true;
    }

    outFilePath.clear();
    return false;
}

bool GodotLoadSettings(GodotLauncherSettings& out)
{
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD value = 0;
    DWORD valueSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"MinimizeToTray", 0, NULL, reinterpret_cast<LPBYTE>(&value), &valueSize) == ERROR_SUCCESS)
    {
        out.minimizeToTray = (value != 0);
    }

    value = 1;
    valueSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"ShowStartPageOnLaunch", 0, NULL, reinterpret_cast<LPBYTE>(&value), &valueSize) == ERROR_SUCCESS)
    {
        out.showStartPageOnLaunch = (value != 0);
    }

    RegCloseKey(hKey);
    return true;
}

bool GodotSaveSettings(const GodotLauncherSettings& in)
{
    HKEY hKey = NULL;
    DWORD disp = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, &disp) != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD minimize = in.minimizeToTray ? 1 : 0;
    DWORD showStart = in.showStartPageOnLaunch ? 1 : 0;
    RegSetValueExW(hKey, L"MinimizeToTray", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&minimize), sizeof(DWORD));
    RegSetValueExW(hKey, L"ShowStartPageOnLaunch", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&showStart), sizeof(DWORD));

    RegCloseKey(hKey);
    return true;
}

bool GodotLoadShortcuts(std::vector<ShortcutItem>& out)
{
    std::wstring filePath = GetShortcutsFilePath();
    if (GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    FILE* file = _wfopen(filePath.c_str(), L"r, ccs=UTF-8");
    if (!file)
    {
        LogToFile("GodotLoadShortcuts: 无法打开 shortcuts.txt");
        return false;
    }

    out.clear();

    WCHAR line[2048];
    while (fgetws(line, sizeof(line) / sizeof(WCHAR), file))
    {
        size_t len = wcslen(line);
        if (len > 0 && line[len - 1] == L'\n') line[len - 1] = L'\0';
        if (line[0] == L'\0') continue;

        std::wstring s = line;
        std::vector<std::wstring> parts;
        size_t start = 0;
        while (true)
        {
            size_t p = s.find(L'|', start);
            if (p == std::wstring::npos)
            {
                parts.push_back(s.substr(start));
                break;
            }
            parts.push_back(s.substr(start, p - start));
            start = p + 1;
        }

        if (parts.size() < 2) continue;

        ShortcutItem item = {0};
        wcsncpy_s(item.name, parts[0].c_str(), _TRUNCATE);
        wcsncpy_s(item.path, parts[1].c_str(), _TRUNCATE);
        item.type = (parts.size() >= 3) ? _wtoi(parts[2].c_str()) : 2;
        wcsncpy_s(item.comment, (parts.size() >= 4) ? parts[3].c_str() : L"", _TRUNCATE);
        wcsncpy_s(item.iconPath, (parts.size() >= 5) ? parts[4].c_str() : L"", _TRUNCATE);
        item.usageCount = (parts.size() >= 6) ? _wtoi(parts[5].c_str()) : 0;
        item.showOnHome = (parts.size() >= 7) ? (_wtoi(parts[6].c_str()) != 0) : false;
        out.push_back(item);
    }

    fclose(file);
    return true;
}

bool GodotSaveShortcuts(const std::vector<ShortcutItem>& shortcuts)
{
    std::wstring outFile;
    if (!EnsureShortcutsFileDir(outFile))
    {
        LogToFile("GodotSaveShortcuts: 无法创建/打开 shortcuts.txt");
        return false;
    }

    FILE* file = _wfopen(outFile.c_str(), L"w, ccs=UTF-8");
    if (!file)
    {
        LogToFile("GodotSaveShortcuts: 无法写入 shortcuts.txt");
        return false;
    }

    for (const auto& shortcut : shortcuts)
    {
        fwprintf(file, L"%s|%s|%d|%s|%s|%d|%d\n",
                 shortcut.name,
                 shortcut.path,
                 shortcut.type,
                 shortcut.comment,
                 shortcut.iconPath,
                 shortcut.usageCount,
                 shortcut.showOnHome ? 1 : 0);
    }

    fclose(file);
    return true;
}

static void FillShortcutItem(ShortcutItem& item, const std::wstring& name, const std::wstring& path, const std::wstring& comment)
{
    wcsncpy_s(item.name, name.c_str(), _TRUNCATE);
    wcsncpy_s(item.path, path.c_str(), _TRUNCATE);
    wcsncpy_s(item.comment, comment.c_str(), _TRUNCATE);
    wcsncpy_s(item.iconPath, path.c_str(), _TRUNCATE);
    item.type = 2;
    item.usageCount = 0;
    item.showOnHome = false;
}

static std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId)
{
    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT, NULL, &path);
    if (FAILED(hr) || !path)
    {
        return L"";
    }
    std::wstring result = path;
    CoTaskMemFree(path);
    return result;
}

static std::wstring GetShortcutDisplayNameFallback(const std::wstring& fullPath)
{
    size_t sep = fullPath.find_last_of(L"\\/");
    std::wstring fileName = (sep == std::wstring::npos) ? fullPath : fullPath.substr(sep + 1);
    size_t dot = fileName.find_last_of(L'.');
    if (dot != std::wstring::npos)
    {
        fileName = fileName.substr(0, dot);
    }
    return fileName;
}

static void CollectAppsFolderShortcuts(std::vector<ShortcutItem>& shortcuts, size_t maxCount, std::set<std::wstring>& seenPaths)
{
    HRESULT hrCo = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bool needUninit = (hrCo == S_OK || hrCo == S_FALSE);

    Microsoft::WRL::ComPtr<IShellItem> appsFolder;
    HRESULT hr = SHCreateItemFromParsingName(L"shell:AppsFolder", NULL, IID_PPV_ARGS(&appsFolder));
    if (SUCCEEDED(hr) && appsFolder)
    {
        Microsoft::WRL::ComPtr<IEnumShellItems> enumItems;
        hr = appsFolder->BindToHandler(NULL, BHID_EnumItems, IID_PPV_ARGS(&enumItems));
        if (SUCCEEDED(hr) && enumItems)
        {
            while (maxCount == 0 || shortcuts.size() < maxCount)
            {
                Microsoft::WRL::ComPtr<IShellItem> child;
                ULONG fetched = 0;
                HRESULT hrNext = enumItems->Next(1, child.GetAddressOf(), &fetched);
                if (hrNext != S_OK || fetched == 0 || !child)
                {
                    break;
                }

                PWSTR displayName = nullptr;
                if (FAILED(child->GetDisplayName(SIGDN_NORMALDISPLAY, &displayName)) || !displayName)
                {
                    continue;
                }

                PWSTR parsingName = nullptr;
                std::wstring launchTarget;
                if (SUCCEEDED(child->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &parsingName)) && parsingName && wcslen(parsingName) > 0)
                {
                    launchTarget = L"shell:AppsFolder\\";
                    launchTarget += parsingName;
                }

                if (!launchTarget.empty())
                {
                    std::wstring lowered = ToLower(launchTarget);
                    if (seenPaths.insert(lowered).second)
                    {
                        ShortcutItem item = {0};
                        FillShortcutItem(item, displayName, launchTarget, L"开始菜单应用列表 (AppsFolder)");
                        item.type = 2;
                        shortcuts.push_back(item);
                    }
                }

                if (parsingName) CoTaskMemFree(parsingName);
                CoTaskMemFree(displayName);
            }
        }
    }

    if (needUninit)
    {
        CoUninitialize();
    }
}

void GodotCollectStartMenuShortcuts(std::vector<ShortcutItem>& out, size_t maxCount)
{
    out.clear();
    std::set<std::wstring> seenPaths;

    std::vector<std::wstring> roots;
    {
        std::wstring p1 = GetKnownFolderPath(FOLDERID_Programs);
        std::wstring p2 = GetKnownFolderPath(FOLDERID_CommonPrograms);
        std::wstring p3 = GetKnownFolderPath(FOLDERID_StartMenu);
        std::wstring p4 = GetKnownFolderPath(FOLDERID_CommonStartMenu);
        if (!p1.empty()) roots.push_back(p1);
        if (!p2.empty()) roots.push_back(p2);
        if (!p3.empty()) roots.push_back(p3);
        if (!p4.empty()) roots.push_back(p4);
    }

    std::function<void(const std::wstring&, const std::wstring&)> walkDirectory;
    walkDirectory = [&](const std::wstring& directory, const std::wstring& rootDirectory)
    {
        if (maxCount > 0 && out.size() >= maxCount)
        {
            return;
        }

        WIN32_FIND_DATAW findData;
        std::wstring searchPattern = directory + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            {
                continue;
            }

            std::wstring fullPath = directory + L"\\" + findData.cFileName;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                walkDirectory(fullPath, rootDirectory);
                if (maxCount > 0 && out.size() >= maxCount)
                {
                    break;
                }
                continue;
            }

            const WCHAR* extension = wcsrchr(findData.cFileName, L'.');
            if (!extension)
            {
                continue;
            }
            const bool isLnk = (_wcsicmp(extension, L".lnk") == 0);
            const bool isUrl = (_wcsicmp(extension, L".url") == 0);
            const bool isAppref = (_wcsicmp(extension, L".appref-ms") == 0);
            if (!isLnk && !isUrl && !isAppref)
            {
                continue;
            }

            std::wstring loweredPath = ToLower(fullPath);
            if (!seenPaths.insert(loweredPath).second)
            {
                continue;
            }

            std::wstring relativeDir;
            if (directory.size() > rootDirectory.size())
            {
                relativeDir = directory.substr(rootDirectory.size() + 1);
            }

            std::wstring comment = L"开始菜单快捷方式";
            if (!relativeDir.empty())
            {
                comment += L": " + relativeDir;
            }

            ShortcutItem item = {0};
            FillShortcutItem(item, GetShortcutDisplayNameFallback(fullPath), fullPath, comment);
            out.push_back(item);
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
    };

    for (const auto& root : roots)
    {
        walkDirectory(root, root);
        if (maxCount > 0 && out.size() >= maxCount)
        {
            break;
        }
    }

    if (maxCount == 0 || out.size() < maxCount)
    {
        CollectAppsFolderShortcuts(out, maxCount, seenPaths);
    }
}

static int FindShortcutByPath(const std::vector<ShortcutItem>& inOut, const std::wstring& pathLower)
{
    for (int i = 0; i < (int)inOut.size(); ++i)
    {
        std::wstring p = inOut[i].path;
        if (ToLower(p) == pathLower)
        {
            return i;
        }
    }
    return -1;
}

int GodotImportStartMenuShortcuts(std::vector<ShortcutItem>& inOut, bool saveChanges)
{
    std::vector<ShortcutItem> collected;
    GodotCollectStartMenuShortcuts(collected, 0);

    int added = 0;
    for (const auto& item : collected)
    {
        std::wstring pLower = ToLower(item.path);
        if (FindShortcutByPath(inOut, pLower) >= 0)
        {
            continue;
        }
        inOut.push_back(item);
        ++added;
    }

    if (saveChanges)
    {
        GodotSaveShortcuts(inOut);
    }
    return added;
}

std::vector<int> GodotSearchShortcuts(const std::vector<ShortcutItem>& all, const std::wstring& query, size_t maxCount)
{
    std::vector<int> indices;
    indices.reserve(std::min(maxCount, all.size()));

    for (int i = 0; i < (int)all.size(); ++i)
    {
        if (maxCount > 0 && indices.size() >= maxCount)
        {
            break;
        }

        std::wstring name = all[i].name;
        std::wstring comment = all[i].comment;
        if (ContainsI(name, query) || ContainsI(comment, query))
        {
            indices.push_back(i);
        }
    }

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        const ShortcutItem& ia = all[a];
        const ShortcutItem& ib = all[b];
        if (ia.usageCount != ib.usageCount) return ia.usageCount > ib.usageCount;
        return _wcsicmp(ia.name, ib.name) < 0;
    });

    return indices;
}

bool GodotExecuteShortcut(const ShortcutItem& item)
{
    if (wcslen(item.path) == 0)
    {
        return false;
    }
    HINSTANCE h = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)h > 32);
}


#pragma once

#ifndef GODOT_LAUNCHER_BACKEND_H
#define GODOT_LAUNCHER_BACKEND_H

#include <string>
#include <vector>
#include "common.h"

struct GodotLauncherSettings {
    bool minimizeToTray = false;
    bool showStartPageOnLaunch = true;
};

bool GodotLoadSettings(GodotLauncherSettings& out);
bool GodotSaveSettings(const GodotLauncherSettings& in);

bool GodotLoadShortcuts(std::vector<ShortcutItem>& out);
bool GodotSaveShortcuts(const std::vector<ShortcutItem>& shortcuts);

void GodotCollectStartMenuShortcuts(std::vector<ShortcutItem>& out, size_t maxCount = 0);
int GodotImportStartMenuShortcuts(std::vector<ShortcutItem>& inOut, bool saveChanges);

std::vector<int> GodotSearchShortcuts(const std::vector<ShortcutItem>& all, const std::wstring& query, size_t maxCount = 200);
bool GodotExecuteShortcut(const ShortcutItem& item);

#endif // GODOT_LAUNCHER_BACKEND_H

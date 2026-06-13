#pragma once

#ifndef GODOT_LAUNCHER_ICON_H
#define GODOT_LAUNCHER_ICON_H

#include <memory>

#include <ui/ui.h>

#include "common.h"

/**
 * @brief 获取快捷方式对应的真实图标。
 * @param shortcut 快捷方式对象
 * @param size 期望图标尺寸
 * @return std::shared_ptr<ui::ImageHandle> 图标句柄，失败返回空
 */
std::shared_ptr<ui::ImageHandle> GodotGetShortcutIconImage(const ShortcutItem& shortcut, int size);

#endif // GODOT_LAUNCHER_ICON_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ui/ui.h>

#include "common.h"
#include "godot_launcher_backend.h"
#include "godot_launcher_icon.h"

namespace {

struct HomeTileItem {
    int actualIndex = -1;
    std::wstring name;
    std::wstring subtitle;
    std::wstring iconText;
    std::shared_ptr<ui::ImageHandle> icon;
};

/**
 * @brief 首页固定区自绘磁贴网格，模拟 Win11 固定应用区的卡片样式。
 */
class HomeTileGridControl : public ui::Control {
public:
    /**
     * @brief 创建首页磁贴网格控件。
     */
    explicit HomeTileGridControl(std::wstring name = L"")
        : ui::Control(std::move(name))
    {
        set_focusable(true);
    }

    /**
     * @brief 设置磁贴数据，并在可能时保留当前选中项。
     */
    void set_tiles(std::vector<HomeTileItem> tiles)
    {
        const int previousActualIndex = selected_actual_index_;
        tiles_ = std::move(tiles);
        selected_actual_index_ = -1;
        for (const HomeTileItem& tile : tiles_)
        {
            if (tile.actualIndex == previousActualIndex)
            {
                selected_actual_index_ = previousActualIndex;
                break;
            }
        }
    }

    /**
     * @brief 设置外部选中的真实快捷方式索引。
     */
    void set_selected_actual_index(int actualIndex)
    {
        selected_actual_index_ = actualIndex;
    }

    /**
     * @brief 设置选中变化回调。
     */
    void set_on_selection_changed(std::function<void(int)> cb)
    {
        on_selection_changed_ = std::move(cb);
    }

    /**
     * @brief 设置双击磁贴回调。
     */
    void set_on_item_double_click(std::function<void(int)> cb)
    {
        on_item_double_click_ = std::move(cb);
    }

    /**
     * @brief 返回磁贴网格的最小尺寸，便于外层滚动容器计算高度。
     */
    ui::Vec2 get_min_size() const override
    {
        const int rows = std::max(1, (int)((tiles_.size() + ColumnCount() - 1) / ColumnCount()));
        const float width = TilePadding() * 2.0f + (TileWidth() * ColumnCount()) + (TileGap() * (ColumnCount() - 1));
        const float height = TilePadding() * 2.0f + (TileHeight() * rows) + (TileGap() * std::max(0, rows - 1));
        return {width, height};
    }

protected:
    /**
     * @brief 绘制圆角磁贴，并显示真实图标、名称与副标题。
     */
    void paint_self(ui::DrawList& out, const ui::Theme& theme) const override
    {
        const ui::Rect bounds = global_rect();
        out.add_rect(bounds, ui::Color::rgba(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);

        for (size_t i = 0; i < tiles_.size(); ++i)
        {
            const HomeTileItem& tile = tiles_[i];
            const ui::Rect rect = TileRectForIndex((int)i);
            const bool selected = (tile.actualIndex == selected_actual_index_);
            const bool hovered = ((int)i == hovered_tile_index_);
            const ui::Color fill = selected
                ? ui::Color::rgba(0.24f, 0.32f, 0.46f, 0.95f)
                : (hovered ? ui::Color::rgba(0.20f, 0.22f, 0.28f, 0.96f) : ui::Color::rgba(0.16f, 0.18f, 0.23f, 0.94f));
            const ui::Color border = selected ? theme.accent_color() : (hovered ? ui::Color::rgba(0.36f, 0.41f, 0.50f, 1.0f) : theme.border_color());

            out.add_rect(rect, fill, 12.0f);
            out.add_border(rect, border, selected ? 2.0f : 1.0f, 12.0f);

            const ui::Rect iconBg{rect.x + 26.0f, rect.y + 14.0f, rect.w - 52.0f, 44.0f};
            if (tile.icon)
            {
                const ui::Rect imageRect{rect.x + ((rect.w - 40.0f) * 0.5f), rect.y + 16.0f, 40.0f, 40.0f};
                out.add_image(imageRect, tile.icon, 1.0f, true);
            }
            else
            {
                out.add_rect(iconBg, ui::Color::rgba(0.23f, 0.36f, 0.60f, 0.92f), 10.0f);
                out.add_text(iconBg, tile.iconText.empty() ? L"APP" : tile.iconText, L"Microsoft YaHei UI", 13.0f, ui::Color::rgba(1.0f, 1.0f, 1.0f, 1.0f), ui::HAlign::Center);
            }

            const ui::Rect nameRect{rect.x + 8.0f, rect.y + 64.0f, rect.w - 16.0f, 24.0f};
            const ui::Rect subtitleRect{rect.x + 8.0f, rect.y + 88.0f, rect.w - 16.0f, 18.0f};
            out.add_text(nameRect, tile.name, L"Microsoft YaHei UI", 14.0f, theme.text_color(), ui::HAlign::Center);
            out.add_text(subtitleRect, tile.subtitle, L"Microsoft YaHei UI", 11.0f, theme.text_dim_color(), ui::HAlign::Center);
        }
    }

    /**
     * @brief 处理磁贴单击选中和双击打开。
     */
    bool on_pointer_event(const ui::PointerEvent& e) override
    {
        const int hitIndex = HitTestTile(e.pos);
        if (e.type == ui::PointerEvent::Type::Move)
        {
            hovered_tile_index_ = hitIndex;
            return (hitIndex >= 0);
        }
        if (e.type == ui::PointerEvent::Type::Down && hitIndex >= 0)
        {
            set_focused(true);
            selected_actual_index_ = tiles_[hitIndex].actualIndex;
            if (on_selection_changed_)
            {
                on_selection_changed_(selected_actual_index_);
            }
            return true;
        }
        if (e.type == ui::PointerEvent::Type::DoubleClick && hitIndex >= 0)
        {
            selected_actual_index_ = tiles_[hitIndex].actualIndex;
            if (on_selection_changed_)
            {
                on_selection_changed_(selected_actual_index_);
            }
            if (on_item_double_click_)
            {
                on_item_double_click_(selected_actual_index_);
            }
            return true;
        }
        return false;
    }

private:
    /**
     * @brief 返回固定区每行显示的磁贴数量。
     */
    static int ColumnCount()
    {
        return 3;
    }

    /**
     * @brief 返回磁贴宽度。
     */
    static float TileWidth()
    {
        return 96.0f;
    }

    /**
     * @brief 返回磁贴高度。
     */
    static float TileHeight()
    {
        return 112.0f;
    }

    /**
     * @brief 返回磁贴之间的间距。
     */
    static float TileGap()
    {
        return 10.0f;
    }

    /**
     * @brief 返回磁贴网格内边距。
     */
    static float TilePadding()
    {
        return 6.0f;
    }

    /**
     * @brief 根据序号计算磁贴矩形。
     */
    ui::Rect TileRectForIndex(int index) const
    {
        const int col = index % ColumnCount();
        const int row = index / ColumnCount();
        const ui::Rect bounds = global_rect();
        const float x = bounds.x + TilePadding() + ((TileWidth() + TileGap()) * col);
        const float y = bounds.y + TilePadding() + ((TileHeight() + TileGap()) * row);
        return {x, y, TileWidth(), TileHeight()};
    }

    /**
     * @brief 根据鼠标位置判断命中的磁贴索引。
     */
    int HitTestTile(const ui::Vec2& pos) const
    {
        for (size_t i = 0; i < tiles_.size(); ++i)
        {
            const ui::Rect rect = TileRectForIndex((int)i);
            if (pos.x >= rect.x && pos.x <= rect.x + rect.w && pos.y >= rect.y && pos.y <= rect.y + rect.h)
            {
                return (int)i;
            }
        }
        return -1;
    }

    std::vector<HomeTileItem> tiles_;
    int selected_actual_index_ = -1;
    int hovered_tile_index_ = -1;
    std::function<void(int)> on_selection_changed_;
    std::function<void(int)> on_item_double_click_;
};

struct LauncherState {
    std::vector<ShortcutItem> shortcuts;
    std::vector<int> searchIndices;
    std::vector<int> homeIndices;
    std::vector<int> recommendedIndices;
    std::vector<int> startMenuIndices;
    GodotLauncherSettings settings;
    std::optional<std::filesystem::file_time_type> shortcutsFileWriteTime;
    ULONGLONG shortcutsWatchTick = 0;
    int selectedActualIndex = -1;
    int editingIndex = -1;
    bool editMode = false;
};

enum class LauncherWindowMode {
    Main,
    Settings,
    ShortcutEdit,
};

struct LaunchArguments {
    LauncherWindowMode mode = LauncherWindowMode::Main;
    std::optional<int> editIndex;
};

/**
 * @brief 按名称查找并转换控件类型。
 */
template <typename T>
T* Find(ui::Control& root, const std::wstring& name)
{
    return dynamic_cast<T*>(root.find_by_name(name));
}

/**
 * @brief 把 C 风格宽字符数组安全转换为 std::wstring。
 */
std::wstring ToWString(const WCHAR* text)
{
    return text ? std::wstring(text) : L"";
}

/**
 * @brief 获取当前可执行文件的完整路径。
 */
std::filesystem::path GetExecutablePath()
{
    wchar_t modulePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (length == 0)
    {
        return std::filesystem::current_path() / L"funny_quick.exe";
    }
    return std::filesystem::path(std::wstring(modulePath, length)).lexically_normal();
}

/**
 * @brief 解析当前进程的启动参数，决定打开主窗口还是独立子窗口。
 */
LaunchArguments ParseLaunchArguments()
{
    LaunchArguments args;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
    {
        return args;
    }

    for (int i = 1; i < argc; ++i)
    {
        const std::wstring arg = argv[i] ? argv[i] : L"";
        if (arg == L"--window=settings")
        {
            args.mode = LauncherWindowMode::Settings;
        }
        else if (arg == L"--window=edit")
        {
            args.mode = LauncherWindowMode::ShortcutEdit;
        }
        else if (arg.rfind(L"--edit-index=", 0) == 0)
        {
            args.editIndex = _wtoi(arg.substr(13).c_str());
        }
        else if (arg == L"--edit-index" && i + 1 < argc)
        {
            args.editIndex = _wtoi(argv[++i]);
        }
    }

    LocalFree(argv);
    return args;
}

/**
 * @brief 把字符串写回 ShortcutItem 的固定长度字段。
 */
void CopyToBuffer(WCHAR* target, size_t targetCount, const std::wstring& value)
{
    if (!target || targetCount == 0)
    {
        return;
    }
    wcsncpy_s(target, targetCount, value.c_str(), _TRUNCATE);
}

/**
 * @brief 判断是否属于开始菜单同步项。
 */
bool IsStartMenuShortcut(const ShortcutItem& item)
{
    return (wcsstr(item.comment, L"开始菜单") != nullptr) || (wcsncmp(item.path, L"shell:AppsFolder\\", 17) == 0);
}

/**
 * @brief 截断磁贴中的长文本，避免标题溢出。
 */
std::wstring Ellipsize(const std::wstring& value, size_t maxChars)
{
    if (value.size() <= maxChars)
    {
        return value;
    }
    if (maxChars <= 3)
    {
        return value.substr(0, maxChars);
    }
    return value.substr(0, maxChars - 3) + L"...";
}

/**
 * @brief 根据快捷方式生成 FileListControl 所需条目。
 */
ui::FileListItem BuildFileItem(const ShortcutItem& item, bool compact)
{
    ui::FileListItem row;
    row.name = item.name;
    row.icon = GodotGetShortcutIconImage(item, compact ? 20 : 56);
    row.is_directory = (item.type == 0);
    row.size = L"使用 " + std::to_wstring(item.usageCount);

    if (item.showOnHome)
    {
        row.size += L" | 首页";
    }

    if (IsStartMenuShortcut(item))
    {
        row.type = L"开始菜单";
        row.modified = item.comment;
    }
    else if (item.type == 1)
    {
        row.type = L"网址";
        row.modified = item.path;
    }
    else if (item.type == 0)
    {
        row.type = L"文件夹";
        row.modified = item.path;
    } 
    else
    {
        row.type = L"程序";
        row.modified = item.path;
    }

    if (!row.icon)
    {
        row.icon_text = (item.type == 0) ? L"DIR" : (item.type == 1 ? L"URL" : L"APP");
    }
    return row;
}

/**
 * @brief 根据快捷方式生成首页磁贴所需数据。
 */
HomeTileItem BuildHomeTileItem(const ShortcutItem& item, int actualIndex)
{
    HomeTileItem tile;
    tile.actualIndex = actualIndex;
    tile.name = Ellipsize(ToWString(item.name), 10);
    tile.subtitle = (item.usageCount > 0)
        ? (L"使用 " + std::to_wstring(item.usageCount))
        : (IsStartMenuShortcut(item) ? L"开始菜单" : L"已固定");
    tile.icon = GodotGetShortcutIconImage(item, 40);
    tile.iconText = (item.type == 0) ? L"DIR" : (item.type == 1 ? L"URL" : L"APP");
    return tile;
}

/**
 * @brief 更新底部状态文本。
 */
void SetStatus(ui::Control& root, const std::wstring& text)
{
    if (auto* status = Find<ui::Label>(root, L"StatusLabel"))
    {
        status->set_text(text);
    }
}

/**
 * @brief 请求关闭当前独立窗口对应的系统窗口。
 */
void RequestCloseWindow(ui::Control& root)
{
    ui::Runtime* runtime = root.runtime();
    if (!runtime)
    {
        return;
    }
    HWND hwnd = static_cast<HWND>(runtime->native_window_handle());
    if (hwnd)
    {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }
}

/**
 * @brief 启动独立的设置/快捷方式编辑窗口进程。
 */
bool LaunchSeparateWindow(ui::Control& root, LauncherWindowMode mode, const std::optional<int>& editIndex = std::nullopt)
{
    const std::filesystem::path exePath = GetExecutablePath();
    std::wstring commandLine = L"\"" + exePath.wstring() + L"\"";
    if (mode == LauncherWindowMode::Settings)
    {
        commandLine += L" --window=settings";
    }
    else if (mode == LauncherWindowMode::ShortcutEdit)
    {
        commandLine += L" --window=edit";
        if (editIndex.has_value())
        {
            commandLine += L" --edit-index=" + std::to_wstring(*editIndex);
        }
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring mutableCommandLine = commandLine;
    if (!CreateProcessW(
            exePath.c_str(),
            mutableCommandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            exePath.parent_path().wstring().c_str(),
            &si,
            &pi))
    {
        SetStatus(root, L"启动独立窗口失败。");
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

/**
 * @brief 刷新右侧详情区域。
 */
void RefreshDetails(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* details = Find<ui::RichTextLabelControl>(root, L"DetailsText");
    auto* openBtn = Find<ui::Button>(root, L"OpenButton");
    auto* editBtn = Find<ui::Button>(root, L"EditButton");
    auto* deleteBtn = Find<ui::Button>(root, L"DeleteButton");
    auto* homeBtn = Find<ui::Button>(root, L"HomeButton");

    if (!details)
    {
        return;
    }

    if (state->selectedActualIndex < 0 || state->selectedActualIndex >= (int)state->shortcuts.size())
    {
        details->set_text(L"未选中快捷方式。\n\n左侧是已固定和开始菜单区域，右侧是搜索/全部结果。\n双击任意列表项可直接启动。");
        if (openBtn) openBtn->set_text(L"打开");
        if (editBtn) editBtn->set_text(L"修改");
        if (deleteBtn) deleteBtn->set_text(L"删除");
        if (homeBtn) homeBtn->set_text(L"加入首页");
        return;
    }

    const ShortcutItem& item = state->shortcuts[state->selectedActualIndex];
    std::wstring text;
    text += L"[b]" + ToWString(item.name) + L"[/b]\n";
    text += L"路径: " + ToWString(item.path) + L"\n";
    text += L"备注: " + ToWString(item.comment) + L"\n";
    text += L"类型: ";
    text += (item.type == 0 ? L"文件夹" : (item.type == 1 ? L"网址" : L"程序"));
    text += L"\n";
    text += L"使用次数: " + std::to_wstring(item.usageCount) + L"\n";
    text += L"首页显示: " + std::wstring(item.showOnHome ? L"是" : L"否");
    details->set_text(text);

    if (homeBtn)
    {
        homeBtn->set_text(item.showOnHome ? L"移出首页" : L"加入首页");
    }
}

/**
 * @brief 按当前搜索词刷新搜索列表。
 */
void RefreshSearchList(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* searchBox = Find<ui::TextBox>(root, L"SearchBox");
    auto* list = Find<ui::FileListControl>(root, L"SearchList");
    auto* summary = Find<ui::Label>(root, L"SearchSummary");
    if (!searchBox || !list)
    {
        return;
    }

    state->searchIndices = GodotSearchShortcuts(state->shortcuts, searchBox->text(), 300);
    std::vector<ui::FileListItem> items;
    items.reserve(state->searchIndices.size());
    for (int index : state->searchIndices)
    {
        items.push_back(BuildFileItem(state->shortcuts[index], true));
    }
    list->set_items(std::move(items));
    if (summary)
    {
        summary->set_text(L"结果: " + std::to_wstring(state->searchIndices.size()) + L" 项");
    }
}

/**
 * @brief 刷新首页固定列表。
 */
void RefreshHomeList(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* list = Find<HomeTileGridControl>(root, L"HomeGrid");
    auto* summary = Find<ui::Label>(root, L"HomeSummary");
    if (!list)
    {
        return;
    }

    state->homeIndices.clear();
    for (int i = 0; i < (int)state->shortcuts.size(); ++i)
    {
        if (state->shortcuts[i].showOnHome)
        {
            state->homeIndices.push_back(i);
        }
    }

    std::sort(state->homeIndices.begin(), state->homeIndices.end(), [&](int a, int b) {
        if (state->shortcuts[a].usageCount != state->shortcuts[b].usageCount)
        {
            return state->shortcuts[a].usageCount > state->shortcuts[b].usageCount;
        }
        return _wcsicmp(state->shortcuts[a].name, state->shortcuts[b].name) < 0;
    });

    std::vector<HomeTileItem> items;
    items.reserve(state->homeIndices.size());
    for (int index : state->homeIndices)
    {
        items.push_back(BuildHomeTileItem(state->shortcuts[index], index));
    }
    list->set_tiles(std::move(items));
    list->set_selected_actual_index(state->selectedActualIndex);
    if (summary)
    {
        summary->set_text(L"首页固定: " + std::to_wstring(state->homeIndices.size()) + L" 项");
    }
}

/**
 * @brief 刷新推荐列表。
 */
void RefreshRecommendedList(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* list = Find<ui::FileListControl>(root, L"RecommendedList");
    auto* summary = Find<ui::Label>(root, L"RecommendedSummary");
    if (!list)
    {
        return;
    }

    state->recommendedIndices.clear();
    for (int i = 0; i < (int)state->shortcuts.size(); ++i)
    {
        state->recommendedIndices.push_back(i);
    }

    std::sort(state->recommendedIndices.begin(), state->recommendedIndices.end(), [&](int a, int b) {
        const ShortcutItem& ia = state->shortcuts[a];
        const ShortcutItem& ib = state->shortcuts[b];
        if (ia.usageCount != ib.usageCount)
        {
            return ia.usageCount > ib.usageCount;
        }
        if (ia.showOnHome != ib.showOnHome)
        {
            return ia.showOnHome && !ib.showOnHome;
        }
        return _wcsicmp(ia.name, ib.name) < 0;
    });

    const int maxCount = std::min<int>((int)state->recommendedIndices.size(), 10);
    std::vector<ui::FileListItem> items;
    items.reserve(maxCount);
    for (int i = 0; i < maxCount; ++i)
    {
        items.push_back(BuildFileItem(state->shortcuts[state->recommendedIndices[i]], true));
    }
    list->set_items(std::move(items));
    if (summary)
    {
        summary->set_text(L"推荐: " + std::to_wstring(maxCount) + L" 项");
    }
}

/**
 * @brief 刷新开始菜单列表。
 */
void RefreshStartMenuList(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* list = Find<ui::FileListControl>(root, L"StartMenuList");
    auto* summary = Find<ui::Label>(root, L"StartMenuSummary");
    if (!list)
    {
        return;
    }

    state->startMenuIndices.clear();
    for (int i = 0; i < (int)state->shortcuts.size(); ++i)
    {
        if (IsStartMenuShortcut(state->shortcuts[i]))
        {
            state->startMenuIndices.push_back(i);
        }
    }

    std::sort(state->startMenuIndices.begin(), state->startMenuIndices.end(), [&](int a, int b) {
        if (state->shortcuts[a].usageCount != state->shortcuts[b].usageCount)
        {
            return state->shortcuts[a].usageCount > state->shortcuts[b].usageCount;
        }
        return _wcsicmp(state->shortcuts[a].name, state->shortcuts[b].name) < 0;
    });

    std::vector<ui::FileListItem> items;
    const int maxCount = std::min<int>((int)state->startMenuIndices.size(), 120);
    items.reserve(maxCount);
    for (int i = 0; i < maxCount; ++i)
    {
        items.push_back(BuildFileItem(state->shortcuts[state->startMenuIndices[i]], true));
    }
    list->set_items(std::move(items));
    if (summary)
    {
        summary->set_text(L"开始菜单: " + std::to_wstring(state->startMenuIndices.size()) + L" 项");
    }
}

/**
 * @brief 根据当前编辑模式切换操作区显隐。
 */
void RefreshEditModeUI(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    if (auto* row = Find<ui::HBox>(root, L"ActionRow"))
    {
        row->set_visible(state->editMode);
    }
    if (auto* button = Find<ui::Button>(root, L"EditModeButton"))
    {
        button->set_text(state->editMode ? L"退出编辑" : L"编辑模式");
    }
}

/**
 * @brief 刷新全部列表和详情。
 */
void RefreshAll(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    RefreshHomeList(root, state);
    RefreshRecommendedList(root, state);
    RefreshStartMenuList(root, state);
    RefreshSearchList(root, state);
    RefreshDetails(root, state);
    RefreshEditModeUI(root, state);
}

/**
 * @brief 打开或关闭设置弹窗，并同步控件值。
 */
void ToggleSettingsPopup(ui::Control& root, const std::shared_ptr<LauncherState>& state, bool visible)
{
    auto* popup = Find<ui::PopupPanelControl>(root, L"SettingsPopup");
    auto* chkMinimize = Find<ui::CheckBox>(root, L"SettingsMinimizeCheck");
    auto* chkHome = Find<ui::CheckBox>(root, L"SettingsStartPageCheck");
    if (!popup)
    {
        return;
    }

    if (visible)
    {
        if (chkMinimize) chkMinimize->set_checked(state->settings.minimizeToTray);
        if (chkHome) chkHome->set_checked(state->settings.showStartPageOnLaunch);
    }
    popup->set_visible(visible);
}

/**
 * @brief 打开或关闭快捷方式编辑弹窗，并同步控件值。
 */
void ToggleEditPopup(ui::Control& root, const std::shared_ptr<LauncherState>& state, bool visible, int actualIndex)
{
    auto* popup = Find<ui::PopupPanelControl>(root, L"EditPopup");
    auto* title = Find<ui::Label>(root, L"EditPopupTitle");
    auto* nameEdit = Find<ui::TextBox>(root, L"EditName");
    auto* pathEdit = Find<ui::TextBox>(root, L"EditPath");
    auto* commentEdit = Find<ui::TextBox>(root, L"EditComment");
    auto* homeCheck = Find<ui::CheckBox>(root, L"EditShowOnHome");
    if (!popup || !title || !nameEdit || !pathEdit || !commentEdit || !homeCheck)
    {
        return;
    }

    state->editingIndex = visible ? actualIndex : -1;
    if (visible && actualIndex >= 0 && actualIndex < (int)state->shortcuts.size())
    {
        const ShortcutItem& item = state->shortcuts[actualIndex];
        title->set_text(L"编辑快捷方式");
        nameEdit->set_text(item.name);
        pathEdit->set_text(item.path);
        commentEdit->set_text(item.comment);
        homeCheck->set_checked(item.showOnHome);
    }
    else if (visible)
    {
        title->set_text(L"新增快捷方式");
        nameEdit->set_text(L"");
        pathEdit->set_text(L"");
        commentEdit->set_text(L"");
        homeCheck->set_checked(false);
    }

    popup->set_visible(visible);
}

/**
 * @brief 尝试打开当前选中的快捷方式。
 */
void OpenSelectedShortcut(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    if (state->selectedActualIndex < 0 || state->selectedActualIndex >= (int)state->shortcuts.size())
    {
        SetStatus(root, L"请先选中一个快捷方式。");
        return;
    }

    ShortcutItem& item = state->shortcuts[state->selectedActualIndex];
    if (GodotExecuteShortcut(item))
    {
        item.usageCount += 1;
        GodotSaveShortcuts(state->shortcuts);
        RefreshAll(root, state);
        SetStatus(root, L"已启动: " + ToWString(item.name));
    }
    else
    {
        SetStatus(root, L"启动失败: " + ToWString(item.path));
    }
}

/**
 * @brief 删除当前选中的快捷方式并保存。
 */
void DeleteSelectedShortcut(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    if (state->selectedActualIndex < 0 || state->selectedActualIndex >= (int)state->shortcuts.size())
    {
        SetStatus(root, L"请先选中要删除的快捷方式。");
        return;
    }

    std::wstring name = state->shortcuts[state->selectedActualIndex].name;
    state->shortcuts.erase(state->shortcuts.begin() + state->selectedActualIndex);
    state->selectedActualIndex = -1;
    GodotSaveShortcuts(state->shortcuts);
    RefreshAll(root, state);
    SetStatus(root, L"已删除: " + name);
}

/**
 * @brief 保存编辑弹窗中的快捷方式数据。
 */
void SaveEditPopup(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* nameEdit = Find<ui::TextBox>(root, L"EditName");
    auto* pathEdit = Find<ui::TextBox>(root, L"EditPath");
    auto* commentEdit = Find<ui::TextBox>(root, L"EditComment");
    auto* homeCheck = Find<ui::CheckBox>(root, L"EditShowOnHome");
    if (!nameEdit || !pathEdit || !commentEdit || !homeCheck)
    {
        return;
    }

    std::wstring name = nameEdit->text();
    std::wstring path = pathEdit->text();
    if (name.empty() || path.empty())
    {
        SetStatus(root, L"名称和路径不能为空。");
        return;
    }

    ShortcutItem item = {};
    CopyToBuffer(item.name, _countof(item.name), name);
    CopyToBuffer(item.path, _countof(item.path), path);
    CopyToBuffer(item.comment, _countof(item.comment), commentEdit->text());
    CopyToBuffer(item.iconPath, _countof(item.iconPath), path);
    item.type = 2;
    item.usageCount = 0;
    item.showOnHome = homeCheck->checked();
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        item.type = 0;
    }
    else if (path.rfind(L"http://", 0) == 0 || path.rfind(L"https://", 0) == 0)
    {
        item.type = 1;
    }

    if (state->editingIndex >= 0 && state->editingIndex < (int)state->shortcuts.size())
    {
        item.usageCount = state->shortcuts[state->editingIndex].usageCount;
        state->shortcuts[state->editingIndex] = item;
        state->selectedActualIndex = state->editingIndex;
        SetStatus(root, L"已保存修改: " + name);
    }
    else
    {
        state->shortcuts.push_back(item);
        state->selectedActualIndex = (int)state->shortcuts.size() - 1;
        SetStatus(root, L"已新增快捷方式: " + name);
    }

    GodotSaveShortcuts(state->shortcuts);
    ToggleEditPopup(root, state, false, -1);
    RefreshAll(root, state);
}

/**
 * @brief 保存独立快捷方式编辑窗口中的内容并关闭窗口。
 */
void SaveEditStandaloneWindow(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* nameEdit = Find<ui::TextBox>(root, L"EditName");
    auto* pathEdit = Find<ui::TextBox>(root, L"EditPath");
    auto* commentEdit = Find<ui::TextBox>(root, L"EditComment");
    auto* homeCheck = Find<ui::CheckBox>(root, L"EditShowOnHome");
    if (!nameEdit || !pathEdit || !commentEdit || !homeCheck)
    {
        return;
    }

    const std::wstring name = nameEdit->text();
    const std::wstring path = pathEdit->text();
    if (name.empty() || path.empty())
    {
        SetStatus(root, L"名称和路径不能为空。");
        return;
    }

    ShortcutItem item = {};
    CopyToBuffer(item.name, _countof(item.name), name);
    CopyToBuffer(item.path, _countof(item.path), path);
    CopyToBuffer(item.comment, _countof(item.comment), commentEdit->text());
    CopyToBuffer(item.iconPath, _countof(item.iconPath), path);
    item.type = 2;
    item.usageCount = 0;
    item.showOnHome = homeCheck->checked();

    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        item.type = 0;
    }
    else if (path.rfind(L"http://", 0) == 0 || path.rfind(L"https://", 0) == 0)
    {
        item.type = 1;
    }

    if (state->editingIndex >= 0 && state->editingIndex < (int)state->shortcuts.size())
    {
        item.usageCount = state->shortcuts[state->editingIndex].usageCount;
        state->shortcuts[state->editingIndex] = item;
    }
    else
    {
        state->shortcuts.push_back(item);
    }

    GodotSaveShortcuts(state->shortcuts);
    RequestCloseWindow(root);
}

/**
 * @brief 绑定独立设置窗口逻辑。
 */
void BindSettingsWindowCallbacks(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* settingsSave = Find<ui::Button>(root, L"SettingsSaveButton");
    auto* settingsCancel = Find<ui::Button>(root, L"SettingsCancelButton");
    auto* settingsMinimize = Find<ui::CheckBox>(root, L"SettingsMinimizeCheck");
    auto* settingsStart = Find<ui::CheckBox>(root, L"SettingsStartPageCheck");

    if (settingsMinimize)
    {
        settingsMinimize->set_checked(state->settings.minimizeToTray);
    }
    if (settingsStart)
    {
        settingsStart->set_checked(state->settings.showStartPageOnLaunch);
    }

    if (settingsSave && settingsMinimize && settingsStart)
    {
        settingsSave->set_on_click([&root, state, settingsMinimize, settingsStart]() {
            state->settings.minimizeToTray = settingsMinimize->checked();
            state->settings.showStartPageOnLaunch = settingsStart->checked();
            GodotSaveSettings(state->settings);
            RequestCloseWindow(root);
        });
    }

    if (settingsCancel)
    {
        settingsCancel->set_on_click([&root]() {
            RequestCloseWindow(root);
        });
    }
}

/**
 * @brief 绑定独立快捷方式编辑窗口逻辑。
 */
void BindShortcutEditWindowCallbacks(ui::Control& root, const std::shared_ptr<LauncherState>& state, const std::optional<int>& editIndex)
{
    auto* title = Find<ui::Label>(root, L"EditPopupTitle");
    auto* nameEdit = Find<ui::TextBox>(root, L"EditName");
    auto* pathEdit = Find<ui::TextBox>(root, L"EditPath");
    auto* commentEdit = Find<ui::TextBox>(root, L"EditComment");
    auto* homeCheck = Find<ui::CheckBox>(root, L"EditShowOnHome");
    auto* editSave = Find<ui::Button>(root, L"EditSaveButton");
    auto* editCancel = Find<ui::Button>(root, L"EditCancelButton");

    state->editingIndex = editIndex.value_or(-1);
    if (state->editingIndex >= 0 && state->editingIndex < (int)state->shortcuts.size())
    {
        const ShortcutItem& item = state->shortcuts[state->editingIndex];
        if (title) title->set_text(L"编辑快捷方式");
        if (nameEdit) nameEdit->set_text(item.name);
        if (pathEdit) pathEdit->set_text(item.path);
        if (commentEdit) commentEdit->set_text(item.comment);
        if (homeCheck) homeCheck->set_checked(item.showOnHome);
    }
    else
    {
        state->editingIndex = -1;
        if (title) title->set_text(L"新增快捷方式");
        if (nameEdit) nameEdit->set_text(L"");
        if (pathEdit) pathEdit->set_text(L"");
        if (commentEdit) commentEdit->set_text(L"");
        if (homeCheck) homeCheck->set_checked(false);
    }

    if (editSave)
    {
        editSave->set_on_click([&root, state]() {
            SaveEditStandaloneWindow(root, state);
        });
    }
    if (editCancel)
    {
        editCancel->set_on_click([&root]() {
            RequestCloseWindow(root);
        });
    }
}

/**
 * @brief 获取 FunnyQuick 可视化场景文件路径。
 */
std::wstring GetLauncherScenePath()
{
    wchar_t modulePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::filesystem::path exePath = (length > 0) ? std::filesystem::path(std::wstring(modulePath, length)) : std::filesystem::current_path();
    return (exePath.parent_path() / L"..\\ui\\funny_quick_main.tscn").lexically_normal().wstring();
}

/**
 * @brief 获取设置窗口场景文件路径。
 */
std::wstring GetSettingsScenePath()
{
    const std::filesystem::path exePath = GetExecutablePath();
    return (exePath.parent_path() / L"..\\ui\\components\\settings_popup.tscn").lexically_normal().wstring();
}

/**
 * @brief 获取快捷方式编辑窗口场景文件路径。
 */
std::wstring GetShortcutEditScenePath()
{
    const std::filesystem::path exePath = GetExecutablePath();
    return (exePath.parent_path() / L"..\\ui\\components\\shortcut_edit_popup.tscn").lexically_normal().wstring();
}

/**
 * @brief 获取 shortcuts.txt 的当前实际读写路径。
 */
std::filesystem::path GetShortcutsDataPath()
{
    const std::filesystem::path exePath = GetExecutablePath();
    const std::filesystem::path moduleDir = exePath.parent_path();
    const std::filesystem::path rootFile = (moduleDir / L"..\\data\\shortcuts.txt").lexically_normal();
    std::error_code ec;
    if (std::filesystem::exists(rootFile, ec))
    {
        return rootFile;
    }
    return (moduleDir / L"data\\shortcuts.txt").lexically_normal();
}

/**
 * @brief 读取 shortcuts.txt 的最后写入时间，供主窗口热刷新使用。
 */
std::optional<std::filesystem::file_time_type> GetShortcutsFileWriteTime()
{
    const std::filesystem::path shortcutsPath = GetShortcutsDataPath();
    std::error_code ec;
    if (!std::filesystem::exists(shortcutsPath, ec))
    {
        return std::nullopt;
    }

    const auto writeTime = std::filesystem::last_write_time(shortcutsPath, ec);
    if (ec)
    {
        return std::nullopt;
    }
    return writeTime;
}

/**
 * @brief 把运行时动态控件挂到 .tscn 场景里的占位面板中。
 */
void AttachDynamicSceneControls(ui::Control& root)
{
    if (auto* host = Find<ui::Panel>(root, L"HomeGridHost"))
    {
        if (!Find<HomeTileGridControl>(root, L"HomeGrid"))
        {
            auto grid = std::make_unique<HomeTileGridControl>(L"HomeGrid");
            grid->set_anchors(0, 0, 1, 1);
            grid->set_offsets(0, 0, 0, 0);
            host->add_child(std::move(grid));
        }
        if (auto* hint = root.find_by_name(L"HomeGridHint"))
        {
            hint->set_visible(false);
        }
    }

    if (auto* host = Find<ui::Panel>(root, L"RecommendedHost"))
    {
        if (!Find<ui::FileListControl>(root, L"RecommendedList"))
        {
            auto list = std::make_unique<ui::FileListControl>(L"RecommendedList");
            list->set_anchors(0, 0, 1, 1);
            list->set_offsets(0, 0, 0, 0);
            host->add_child(std::move(list));
        }
        if (auto* hint = root.find_by_name(L"RecommendedHint"))
        {
            hint->set_visible(false);
        }
    }

    if (auto* host = Find<ui::Panel>(root, L"StartMenuHost"))
    {
        if (!Find<ui::FileListControl>(root, L"StartMenuList"))
        {
            auto list = std::make_unique<ui::FileListControl>(L"StartMenuList");
            list->set_anchors(0, 0, 1, 1);
            list->set_offsets(0, 0, 0, 0);
            host->add_child(std::move(list));
        }
        if (auto* hint = root.find_by_name(L"StartMenuHint"))
        {
            hint->set_visible(false);
        }
    }

    if (auto* host = Find<ui::Panel>(root, L"SearchListHost"))
    {
        if (!Find<ui::FileListControl>(root, L"SearchList"))
        {
            auto list = std::make_unique<ui::FileListControl>(L"SearchList");
            list->set_anchors(0, 0, 1, 1);
            list->set_offsets(0, 0, 0, 0);
            host->add_child(std::move(list));
        }
    }
}

/**
 * @brief 用代码构建主界面控件树，作为 .tscn 不可用时的兜底。
 */
std::unique_ptr<ui::Control> BuildLauncherUICodeFallback()
{
    auto root = std::make_unique<ui::Control>(L"LauncherRoot");
    root->set_anchors(0, 0, 1, 1);

    auto bg = std::make_unique<ui::Panel>(L"Desktop");
    bg->set_anchors(0, 0, 1, 1);
    bg->set_bg_color(ui::Color::rgba(0.08f, 0.09f, 0.11f, 1.0f));

    auto mainVBox = std::make_unique<ui::VBox>(L"MainVBox");
    mainVBox->set_anchors(0, 0, 1, 1);
    mainVBox->set_offsets(18, 18, -18, -18);
    mainVBox->set_spacing(10.0f);

    auto title = std::make_unique<ui::Label>(L"TitleLabel");
    title->set_text(L"Funny Quick - Godot UI 版");
    title->set_font_size(26.0f);
    title->set_custom_min_size(0, 34);

    auto subtitle = std::make_unique<ui::Label>(L"SubTitleLabel");
    subtitle->set_text(L"整套 Direct2D / Godot 风格界面。支持搜索、开始菜单同步、真实图标、首页磁贴和设置保存。");
    subtitle->set_custom_min_size(0, 24);

    auto toolbar = std::make_unique<ui::HBox>(L"Toolbar");
    toolbar->set_spacing(8.0f);
    toolbar->set_custom_min_size(0, 46);

    auto searchBox = std::make_unique<ui::TextBox>(L"SearchBox");
    searchBox->set_placeholder(L"输入名称搜索，例如 cloud");
    searchBox->set_size_flags_horizontal(3);
    searchBox->set_custom_min_size(420, 40);

    auto syncButton = std::make_unique<ui::Button>(L"SyncButton");
    syncButton->set_text(L"同步开始菜单");
    syncButton->set_custom_min_size(140, 40);

    auto addButton = std::make_unique<ui::Button>(L"AddButton");
    addButton->set_text(L"新增");
    addButton->set_custom_min_size(88, 40);

    auto settingsButton = std::make_unique<ui::Button>(L"SettingsButton");
    settingsButton->set_text(L"设置");
    settingsButton->set_custom_min_size(88, 40);

    auto editModeButton = std::make_unique<ui::Button>(L"EditModeButton");
    editModeButton->set_text(L"编辑模式");
    editModeButton->set_custom_min_size(110, 40);

    toolbar->add_child(std::move(searchBox));
    toolbar->add_child(std::move(syncButton));
    toolbar->add_child(std::move(addButton));
    toolbar->add_child(std::move(settingsButton));
    toolbar->add_child(std::move(editModeButton));

    auto split = std::make_unique<ui::SplitContainerControl>(L"MainSplit", false);
    split->set_size_flags_vertical(3);
    split->set_split_offset(340.0f);

    auto leftPanel = std::make_unique<ui::Panel>(L"LeftPanel");
    leftPanel->set_bg_color(ui::Color::rgba(0.09f, 0.10f, 0.13f, 0.95f));
    auto leftScroll = std::make_unique<ui::ScrollContainer>(L"LeftScroll");
    leftScroll->set_anchors(0, 0, 1, 1);
    leftScroll->set_offsets(10, 10, -10, -10);
    auto leftVBox = std::make_unique<ui::VBox>(L"LeftVBox");
    leftVBox->set_spacing(12.0f);
    leftVBox->set_custom_min_size(312, 0);

    auto homeSection = std::make_unique<ui::Panel>(L"HomeSection");
    homeSection->set_bg_color(ui::Color::rgba(0.14f, 0.15f, 0.19f, 0.96f));
    auto homeVBox = std::make_unique<ui::VBox>(L"HomeVBox");
    homeVBox->set_anchors(0, 0, 1, 1);
    homeVBox->set_offsets(12, 12, -12, -12);
    homeVBox->set_spacing(6.0f);

    auto homeTitle = std::make_unique<ui::Label>(L"HomeTitle");
    homeTitle->set_text(L"已固定");
    homeTitle->set_font_size(20.0f);

    auto homeSummary = std::make_unique<ui::Label>(L"HomeSummary");
    homeSummary->set_text(L"首页固定: 0 项");
    homeSummary->set_custom_min_size(0, 22);

    auto homeHint = std::make_unique<ui::Label>(L"HomeHint");
    homeHint->set_text(L"单击选中，双击立即打开。");
    homeHint->set_custom_min_size(0, 20);

    auto homeGrid = std::make_unique<HomeTileGridControl>(L"HomeGrid");
    homeGrid->set_custom_min_size(0, 132);

    homeVBox->add_child(std::move(homeTitle));
    homeVBox->add_child(std::move(homeSummary));
    homeVBox->add_child(std::move(homeHint));
    homeVBox->add_child(std::move(homeGrid));
    homeSection->add_child(std::move(homeVBox));

    auto recommendedSection = std::make_unique<ui::Panel>(L"RecommendedSection");
    recommendedSection->set_bg_color(ui::Color::rgba(0.13f, 0.145f, 0.18f, 0.96f));
    auto recommendedVBox = std::make_unique<ui::VBox>(L"RecommendedVBox");
    recommendedVBox->set_anchors(0, 0, 1, 1);
    recommendedVBox->set_offsets(12, 12, -12, -12);
    recommendedVBox->set_spacing(6.0f);

    auto recommendedTitle = std::make_unique<ui::Label>(L"RecommendedTitle");
    recommendedTitle->set_text(L"推荐");
    recommendedTitle->set_font_size(18.0f);

    auto recommendedSummary = std::make_unique<ui::Label>(L"RecommendedSummary");
    recommendedSummary->set_text(L"推荐: 0 项");
    recommendedSummary->set_custom_min_size(0, 22);

    auto recommendedList = std::make_unique<ui::FileListControl>(L"RecommendedList");
    recommendedList->set_custom_min_size(0, 150);
    recommendedList->set_size_flags_vertical(1);

    recommendedVBox->add_child(std::move(recommendedTitle));
    recommendedVBox->add_child(std::move(recommendedSummary));
    recommendedVBox->add_child(std::move(recommendedList));
    recommendedSection->add_child(std::move(recommendedVBox));

    auto startMenuSection = std::make_unique<ui::Panel>(L"StartMenuSection");
    startMenuSection->set_bg_color(ui::Color::rgba(0.13f, 0.145f, 0.18f, 0.96f));
    auto startMenuVBox = std::make_unique<ui::VBox>(L"StartMenuVBox");
    startMenuVBox->set_anchors(0, 0, 1, 1);
    startMenuVBox->set_offsets(12, 12, -12, -12);
    startMenuVBox->set_spacing(6.0f);

    auto startMenuTitle = std::make_unique<ui::Label>(L"StartMenuTitle");
    startMenuTitle->set_text(L"开始菜单程序");
    startMenuTitle->set_font_size(18.0f);

    auto startMenuSummary = std::make_unique<ui::Label>(L"StartMenuSummary");
    startMenuSummary->set_text(L"开始菜单: 0 项");
    startMenuSummary->set_custom_min_size(0, 22);

    auto startMenuList = std::make_unique<ui::FileListControl>(L"StartMenuList");
    startMenuList->set_custom_min_size(0, 240);
    startMenuList->set_size_flags_vertical(1);

    startMenuVBox->add_child(std::move(startMenuTitle));
    startMenuVBox->add_child(std::move(startMenuSummary));
    startMenuVBox->add_child(std::move(startMenuList));
    startMenuSection->add_child(std::move(startMenuVBox));

    leftVBox->add_child(std::move(homeSection));
    leftVBox->add_child(std::move(recommendedSection));
    leftVBox->add_child(std::move(startMenuSection));
    leftScroll->add_child(std::move(leftVBox));
    leftPanel->add_child(std::move(leftScroll));

    auto rightPanel = std::make_unique<ui::Panel>(L"RightPanel");
    rightPanel->set_bg_color(ui::Color::rgba(0.10f, 0.11f, 0.14f, 0.95f));
    auto rightVBox = std::make_unique<ui::VBox>(L"RightVBox");
    rightVBox->set_anchors(0, 0, 1, 1);
    rightVBox->set_offsets(10, 10, -10, -10);
    rightVBox->set_spacing(8.0f);

    auto resultsTitle = std::make_unique<ui::Label>(L"ResultsTitle");
    resultsTitle->set_text(L"搜索结果 / 全部快捷方式");
    resultsTitle->set_font_size(20.0f);

    auto resultsSummary = std::make_unique<ui::Label>(L"SearchSummary");
    resultsSummary->set_text(L"结果: 0 项");

    auto resultsList = std::make_unique<ui::FileListControl>(L"SearchList");
    resultsList->set_custom_min_size(0, 320);
    resultsList->set_size_flags_vertical(3);

    auto actionRow = std::make_unique<ui::HBox>(L"ActionRow");
    actionRow->set_spacing(8.0f);
    actionRow->set_custom_min_size(0, 40);

    auto openButton = std::make_unique<ui::Button>(L"OpenButton");
    openButton->set_text(L"打开");
    openButton->set_custom_min_size(88, 38);

    auto editButton = std::make_unique<ui::Button>(L"EditButton");
    editButton->set_text(L"修改");
    editButton->set_custom_min_size(88, 38);

    auto deleteButton = std::make_unique<ui::Button>(L"DeleteButton");
    deleteButton->set_text(L"删除");
    deleteButton->set_custom_min_size(88, 38);

    auto homeButton = std::make_unique<ui::Button>(L"HomeButton");
    homeButton->set_text(L"加入首页");
    homeButton->set_custom_min_size(110, 38);

    actionRow->add_child(std::move(openButton));
    actionRow->add_child(std::move(editButton));
    actionRow->add_child(std::move(deleteButton));
    actionRow->add_child(std::move(homeButton));
    actionRow->set_visible(false);

    auto detailsTitle = std::make_unique<ui::Label>(L"DetailsTitle");
    detailsTitle->set_text(L"详情");

    auto detailsText = std::make_unique<ui::RichTextLabelControl>(L"DetailsText");
    detailsText->set_custom_min_size(0, 180);

    rightVBox->add_child(std::move(resultsTitle));
    rightVBox->add_child(std::move(resultsSummary));
    rightVBox->add_child(std::move(resultsList));
    rightVBox->add_child(std::move(actionRow));
    rightVBox->add_child(std::move(detailsTitle));
    rightVBox->add_child(std::move(detailsText));
    rightPanel->add_child(std::move(rightVBox));

    split->add_child(std::move(leftPanel));
    split->add_child(std::move(rightPanel));

    auto status = std::make_unique<ui::Label>(L"StatusLabel");
    status->set_text(L"就绪。");
    status->set_custom_min_size(0, 24);

    mainVBox->add_child(std::move(title));
    mainVBox->add_child(std::move(subtitle));
    mainVBox->add_child(std::move(toolbar));
    mainVBox->add_child(std::move(split));
    mainVBox->add_child(std::move(status));
    bg->add_child(std::move(mainVBox));

    root->add_child(std::move(bg));
    return root;
}

/**
 * @brief 优先从 .tscn 场景加载 FunnyQuick UI，失败时退回代码构建。
 */
std::unique_ptr<ui::Control> BuildLauncherUI()
{
    const std::wstring scenePath = GetLauncherScenePath();
    std::error_code ec;
    if (std::filesystem::exists(scenePath, ec))
    {
        std::unique_ptr<ui::Control> sceneRoot = ui::load_tscn_ui(scenePath);
        if (sceneRoot)
        {
            AttachDynamicSceneControls(*sceneRoot);
            return sceneRoot;
        }
    }
    return BuildLauncherUICodeFallback();
}

/**
 * @brief 绑定主界面的交互逻辑。
 */
void BindCallbacks(ui::Control& root, const std::shared_ptr<LauncherState>& state)
{
    auto* searchBox = Find<ui::TextBox>(root, L"SearchBox");
    auto* searchList = Find<ui::FileListControl>(root, L"SearchList");
    auto* homeGrid = Find<HomeTileGridControl>(root, L"HomeGrid");
    auto* recommendedList = Find<ui::FileListControl>(root, L"RecommendedList");
    auto* startMenuList = Find<ui::FileListControl>(root, L"StartMenuList");
    auto* syncButton = Find<ui::Button>(root, L"SyncButton");
    auto* addButton = Find<ui::Button>(root, L"AddButton");
    auto* settingsButton = Find<ui::Button>(root, L"SettingsButton");
    auto* editModeButton = Find<ui::Button>(root, L"EditModeButton");
    auto* openButton = Find<ui::Button>(root, L"OpenButton");
    auto* editButton = Find<ui::Button>(root, L"EditButton");
    auto* deleteButton = Find<ui::Button>(root, L"DeleteButton");
    auto* homeButton = Find<ui::Button>(root, L"HomeButton");
    auto* settingsSave = Find<ui::Button>(root, L"SettingsSaveButton");
    auto* settingsCancel = Find<ui::Button>(root, L"SettingsCancelButton");
    auto* editSave = Find<ui::Button>(root, L"EditSaveButton");
    auto* editCancel = Find<ui::Button>(root, L"EditCancelButton");
    auto* settingsPopup = Find<ui::PopupPanelControl>(root, L"SettingsPopup");
    auto* editPopup = Find<ui::PopupPanelControl>(root, L"EditPopup");
    auto* settingsMinimize = Find<ui::CheckBox>(root, L"SettingsMinimizeCheck");
    auto* settingsStart = Find<ui::CheckBox>(root, L"SettingsStartPageCheck");

    if (searchBox)
    {
        searchBox->set_on_text_changed([&root, state](const std::wstring&) {
            RefreshSearchList(root, state);
            RefreshDetails(root, state);
        });
        searchBox->set_on_submit([&root, state]() {
            RefreshSearchList(root, state);
            SetStatus(root, L"已刷新搜索结果。");
        });
    }

    if (searchList)
    {
        searchList->set_on_selection_changed([&root, state](int index) {
            state->selectedActualIndex = (index >= 0 && index < (int)state->searchIndices.size()) ? state->searchIndices[index] : -1;
            RefreshDetails(root, state);
        });
        searchList->set_on_item_double_click([&root, state](int index) {
            state->selectedActualIndex = (index >= 0 && index < (int)state->searchIndices.size()) ? state->searchIndices[index] : -1;
            OpenSelectedShortcut(root, state);
        });
    }

    if (homeGrid)
    {
        homeGrid->set_on_selection_changed([&root, state](int actualIndex) {
            state->selectedActualIndex = actualIndex;
            RefreshDetails(root, state);
        });
        homeGrid->set_on_item_double_click([&root, state](int actualIndex) {
            state->selectedActualIndex = actualIndex;
            OpenSelectedShortcut(root, state);
        });
    }

    if (recommendedList)
    {
        recommendedList->set_on_selection_changed([&root, state](int index) {
            state->selectedActualIndex = (index >= 0 && index < (int)state->recommendedIndices.size()) ? state->recommendedIndices[index] : -1;
            RefreshDetails(root, state);
        });
        recommendedList->set_on_item_double_click([&root, state](int index) {
            state->selectedActualIndex = (index >= 0 && index < (int)state->recommendedIndices.size()) ? state->recommendedIndices[index] : -1;
            OpenSelectedShortcut(root, state);
        });
    }

    if (startMenuList)
    {
        startMenuList->set_on_selection_changed([&root, state](int index) {
            state->selectedActualIndex = (index >= 0 && index < (int)state->startMenuIndices.size()) ? state->startMenuIndices[index] : -1;
            RefreshDetails(root, state);
        });
        startMenuList->set_on_item_double_click([&root, state](int index) {
            state->selectedActualIndex = (index >= 0 && index < (int)state->startMenuIndices.size()) ? state->startMenuIndices[index] : -1;
            OpenSelectedShortcut(root, state);
        });
    }

    if (syncButton)
    {
        syncButton->set_on_click([&root, state]() {
            int added = GodotImportStartMenuShortcuts(state->shortcuts, true);
            RefreshAll(root, state);
            SetStatus(root, L"同步开始菜单完成，新增 " + std::to_wstring(added) + L" 项。");
        });
    }

    if (addButton)
    {
        addButton->set_on_click([&root, state]() {
            if (LaunchSeparateWindow(root, LauncherWindowMode::ShortcutEdit))
            {
                SetStatus(root, L"已打开独立新增快捷方式窗口。");
            }
        });
    }

    if (settingsButton)
    {
        settingsButton->set_on_click([&root, state]() {
            if (LaunchSeparateWindow(root, LauncherWindowMode::Settings))
            {
                SetStatus(root, L"已打开独立设置窗口。");
            }
        });
    }

    if (editModeButton)
    {
        editModeButton->set_on_click([&root, state]() {
            state->editMode = !state->editMode;
            RefreshEditModeUI(root, state);
            SetStatus(root, state->editMode ? L"已进入编辑模式。" : L"已退出编辑模式。");
        });
    }

    if (openButton)
    {
        openButton->set_on_click([&root, state]() {
            OpenSelectedShortcut(root, state);
        });
    }

    if (editButton)
    {
        editButton->set_on_click([&root, state]() {
            if (state->selectedActualIndex < 0)
            {
                SetStatus(root, L"请先选中要修改的快捷方式。");
                return;
            }
            if (LaunchSeparateWindow(root, LauncherWindowMode::ShortcutEdit, state->selectedActualIndex))
            {
                SetStatus(root, L"已打开独立快捷方式编辑窗口。");
            }
        });
    }

    if (deleteButton)
    {
        deleteButton->set_on_click([&root, state]() {
            DeleteSelectedShortcut(root, state);
        });
    }

    if (homeButton)
    {
        homeButton->set_on_click([&root, state]() {
            if (state->selectedActualIndex < 0 || state->selectedActualIndex >= (int)state->shortcuts.size())
            {
                SetStatus(root, L"请先选中快捷方式。");
                return;
            }
            ShortcutItem& item = state->shortcuts[state->selectedActualIndex];
            item.showOnHome = !item.showOnHome;
            GodotSaveShortcuts(state->shortcuts);
            RefreshAll(root, state);
            SetStatus(root, item.showOnHome ? L"已加入首页。" : L"已移出首页。");
        });
    }

    if (settingsSave && settingsMinimize && settingsStart)
    {
        settingsSave->set_on_click([&root, state, settingsMinimize, settingsStart]() {
            state->settings.minimizeToTray = settingsMinimize->checked();
            state->settings.showStartPageOnLaunch = settingsStart->checked();
            if (GodotSaveSettings(state->settings))
            {
                SetStatus(root, L"设置已保存。");
            }
            else
            {
                SetStatus(root, L"设置保存失败。");
            }
            ToggleSettingsPopup(root, state, false);
        });
    }

    if (settingsCancel)
    {
        settingsCancel->set_on_click([&root, state]() {
            ToggleSettingsPopup(root, state, false);
        });
    }

    if (settingsPopup)
    {
        settingsPopup->set_on_close([&root, state]() {
            ToggleSettingsPopup(root, state, false);
        });
    }

    if (editSave)
    {
        editSave->set_on_click([&root, state]() {
            SaveEditPopup(root, state);
        });
    }

    if (editCancel)
    {
        editCancel->set_on_click([&root, state]() {
            ToggleEditPopup(root, state, false, -1);
        });
    }

    if (editPopup)
    {
        editPopup->set_on_close([&root, state]() {
            ToggleEditPopup(root, state, false, -1);
        });
    }
}

} // namespace

/**
 * @brief Godot UI 版本的程序入口。
 */
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    const LaunchArguments launchArgs = ParseLaunchArguments();
    auto state = std::make_shared<LauncherState>();
    GodotLoadSettings(state->settings);
    GodotLoadShortcuts(state->shortcuts);
    state->shortcutsFileWriteTime = GetShortcutsFileWriteTime();

    ui::Runtime runtime;
    ui::Theme theme = ui::Theme::make_default();
    theme.set_panel_bg(ui::Color::rgba(0.08f, 0.09f, 0.11f, 1.0f));
    theme.set_surface_bg(ui::Color::rgba(0.13f, 0.14f, 0.18f, 1.0f));
    theme.set_surface_alt_bg(ui::Color::rgba(0.16f, 0.18f, 0.23f, 1.0f));
    theme.set_border_color(ui::Color::rgba(0.24f, 0.27f, 0.33f, 1.0f));
    theme.set_text_color(ui::Color::rgba(0.94f, 0.95f, 0.97f, 1.0f));
    theme.set_text_dim_color(ui::Color::rgba(0.68f, 0.72f, 0.79f, 1.0f));
    theme.set_button_bg_color(ui::Color::rgba(0.18f, 0.20f, 0.25f, 1.0f));
    theme.set_button_hover_color(ui::Color::rgba(0.23f, 0.26f, 0.32f, 1.0f));
    theme.set_button_pressed_color(ui::Color::rgba(0.27f, 0.31f, 0.38f, 1.0f));
    theme.set_focus_border_color(ui::Color::rgba(0.41f, 0.66f, 0.98f, 1.0f));
    theme.set_accent_color(ui::Color::rgba(0.35f, 0.62f, 0.98f, 1.0f));
    theme.set_accent_soft_color(ui::Color::rgba(0.27f, 0.40f, 0.62f, 0.55f));
    theme.set_corner_radius(10.0f);
    theme.set_spacing(10.0f);
    theme.set_padding_x(12.0f);
    theme.set_padding_y(8.0f);
    runtime.set_theme(theme);

    std::unique_ptr<ui::Control> root;
    ui::RuntimeConfig cfg;

    if (launchArgs.mode == LauncherWindowMode::Settings)
    {
        root = ui::load_tscn_ui(GetSettingsScenePath());
        if (!root)
        {
            return 1;
        }
        BindSettingsWindowCallbacks(*root, state);
        cfg.width = 520;
        cfg.height = 240;
        cfg.title = L"Funny Quick - 设置";
    }
    else if (launchArgs.mode == LauncherWindowMode::ShortcutEdit)
    {
        root = ui::load_tscn_ui(GetShortcutEditScenePath());
        if (!root)
        {
            return 1;
        }
        BindShortcutEditWindowCallbacks(*root, state, launchArgs.editIndex);
        cfg.width = 660;
        cfg.height = 320;
        cfg.title = L"Funny Quick - 快捷方式";
    }
    else
    {
        root = BuildLauncherUI();
        BindCallbacks(*root, state);
        RefreshAll(*root, state);
        cfg.width = 1280;
        cfg.height = 840;
        cfg.title = L"Funny Quick (Godot UI)";
    }

    runtime.set_root(std::move(root));

    if (launchArgs.mode == LauncherWindowMode::Main)
    {
        runtime.set_on_tick([&runtime, state]() {
            const ULONGLONG now = GetTickCount64();
            if (now - state->shortcutsWatchTick < 800)
            {
                return;
            }
            state->shortcutsWatchTick = now;

            const auto currentWriteTime = GetShortcutsFileWriteTime();
            if (currentWriteTime == state->shortcutsFileWriteTime)
            {
                return;
            }

            state->shortcutsFileWriteTime = currentWriteTime;
            ui::Control* rootControl = runtime.root();
            if (!rootControl)
            {
                return;
            }

            GodotLoadShortcuts(state->shortcuts);
            RefreshAll(*rootControl, state);
            SetStatus(*rootControl, L"已自动刷新快捷方式数据。");
        });
    }

    cfg.tick_ms = 16;
    return runtime.run(cfg);
}

#ifndef INTERNATIONALIZATION_H
#define INTERNATIONALIZATION_H

#ifdef LANGUAGE_CHINESE
// ===========================================
// 中文界面配置 - Chinese Interface Configuration
// ===========================================

// 快捷项名称 - Shortcut Names
#define ITEM_DOCUMENTS         L"文档"
#define ITEM_PICTURES          L"图片"
#define ITEM_MUSIC             L"音乐"
#define ITEM_VIDEOS            L"视频"
#define ITEM_RECYCLE_BIN       L"回收站"
#define ITEM_CONTROL_PANEL     L"控制面板"
#define ITEM_CALCULATOR        L"计算器"
#define ITEM_NOTEPAD           L"记事本"
#define ITEM_PAINT             L"画图"
#define ITEM_CMD               L"命令提示符"
#define ITEM_POWERSHELL        L"PowerShell"

// 搜索结果显示 - Search Results Display
#define SEARCH_PREFIX_DIR      L"DIR: "
#define SEARCH_PREFIX_URL      L"URL: "
#define SEARCH_PREFIX_APP      L"APP: "
#define SEARCH_NO_RESULTS      L"未找到匹配项"
#define SEARCH_COLLECTION      L"收藏:"

// 按钮文本 - Button Text
#define BTN_EXIT_CALC          L"退出计算"
#define BTN_EXIT               L"退出"
#define BTN_SETTINGS           L"设置"
#define BTN_EXIT_BOOKMARK      L"退出"
#define LABEL_CALC             L"计算:(输入q退出)"
#define LABEL_SEARCH           L"搜索:"
#define LABEL_URL              L"网址:"
#define MSG_INVALID_URL        L"请输入有效的网址"
#define MSG_URL_EXISTS         L"该网址已存在于收藏中"
#define MSG_ADD_BOOKMARK_FAIL  L"添加网址失败"
#define MSG_CLEAR_HISTORY      L"确定要清空所有计算历史吗？"
#define MSG_CALC_FAILED        L"无法计算表达式"
  #define MSG_CALC_EXCEPTION     L"表达式计算异常"
  #define MENU_EXIT_CALC         L"退出计算模式"
  #define MENU_EXIT_PROGRAM      L"退出程序"
  #define MSG_OPTIONS_IN_DEVELOPMENT L"选项功能正在开发中..."
  #define DLG_HELP_TITLE         L"帮助"
#define DLG_CALC_ERROR         L"计算错误"
#define BTN_ADD                L"添加"
#define BTN_UPDATE             L"更新"
#define BTN_DELETE             L"删除"
#define BTN_CLOSE              L"关闭"
#define BTN_SHOW_WINDOW        L"显示窗口"

// 标签文本 - Label Text
#define LABEL_SEARCH           L"搜索:"
#define LABEL_CALC             L"计算:(输入q退出)"
#define LABEL_URL              L"网址:"
#define LABEL_NAME             L"名称:"
#define LABEL_FAST_LAUNCH      L"快速启动"

// 对话框标题 - Dialog Titles
#define DLG_BOOKMARK_MANAGE    L"网址收藏管理"
#define DLG_ADD_FAILED         L"添加网址失败"
#define DLG_DELETE_CONFIRM     L"确认删除"
#define DLG_SYNC_FAILED        L"同步失败"
#define DLG_SYNC_SUCCESS       L"同步完成"
#define DLG_CALC_ERROR         L"计算错误"
#define DLG_HELP_INFO          L"帮助信息"
#define DLG_DEVELOPING         L"提示"

// 错误信息 - Error Messages
#define MSG_INVALID_URL        L"请输入有效的网址"
#define MSG_URL_EXISTS         L"该网址已存在于收藏中"
#define MSG_CHROME_NOT_FOUND   L"未找到Chrome书签文件，请确保Chrome已安装并至少添加过一个书签"
#define MSG_CHROME_READ_ERROR  L"无法读取Chrome书签文件，请确保Chrome已关闭"
#define MSG_USER_DATA_ERROR    L"无法获取用户数据目录"
#define MSG_CALC_FAILED        L"无法计算表达式"
#define MSG_CALC_EXCEPTION     L"表达式计算异常"
#define MSG_INPUT_EMPTY        L"请输入名称和URL"
#define MSG_INVALID_INPUT_URL  L"请输入有效的URL"
#define MSG_SELECT_UPDATE      L"请选择要更新的网址"
#define MSG_SELECT_DELETE      L"请选择要删除的网址"
#define MSG_DELETE_CONFIRM_TEXT L"确定要删除选中的网址吗？"
#define MSG_CLEAR_HISTORY      L"确定要清空所有计算历史吗？"
#define MSG_ADD_URL_HINT       L"请输入有效的网址"
#define MSG_SYNC_INFO          L"成功从Chrome同步了 %d 个新书签"

// 设置菜单项 - Settings Menu Items
#define MENU_BOOKMARK_MANAGE   L"网址管理..."
#define MENU_OPTIONS           L"选项设置..."
#define MENU_HELP              L"帮助..."
#define MENU_EXIT_CALC         L"退出计算模式"
#define MENU_EXIT_PROGRAM      L"退出程序"

// 上下文菜单项 - Context Menu Items
#define CONTEXT_DELETE_ITEM    L"删除此项"
#define CONTEXT_CLEAR_ALL      L"清空历史"
#define CONTEXT_DELETE_BOOKMARK L"删除此项"
#define CONTEXT_SYNC_CHROME    L"同步Chrome书签"

// 帮助内容 - Help Content
#define HELP_CALCULATOR_TITLE  L"计算器模式使用帮助\n\n"
#define HELP_CALCULATOR_BASIC  L"基本运算：\n• 输入表达式进行计算，如：2+3*4\n• 支持加减乘除：+ - * /\n• 支持括号：() [] {}\n• 支持小数：3.14\n\n"
#define HELP_CALCULATOR_ADVANCED L"高级功能：\n• 添加注释：输入 表达式 # 注释内容\n  例如：1+2 #买面包\n  例如：10*5 #买水果\n• 注释会自动右对齐显示\n\n"
#define HELP_CALCULATOR_OPERATIONS L"操作方法：\n• 在输入框输入表达式，按Enter计算\n• 计算历史会显示在列表中\n• 最多保存50条历史记录\n• 点击历史记录可复制到输入框\n• 输入\"js\"可切换到网址收藏模式\n• 输入\"q\"可退出计算模式"

#define HELP_BOOKMARK_TITLE   L"网址收藏模式使用帮助\n\n"
#define HELP_BOOKMARK_BASIC   L"基本操作：\n• 收藏网址：输入\"网址名=网址\"保存\n  例如：百度=https://www.baidu.com\n• 打开网址：双击列表中的网址项\n• 删除网址：选中网址后按Delete键\n• 修改网址：双击网址进行编辑\n\n"
#define HELP_BOOKMARK_SHORTCUTS L"快捷操作：\n• 右键点击网址可弹出操作菜单\n• 支持复制网址到剪贴板\n• 支持导入/导出收藏\n• 支持批量操作\n\n"
#define HELP_BOOKMARK_FORMAT  L"输入格式：\n• 格式：网址名=网址\n• 等号两边不要有空格\n• 网址必须包含http://或https://\n\n"
#define HELP_BOOKMARK_OPERATIONS L"操作方法：\n• 在输入框输入网址信息\n• 计算历史会记录操作过程\n• 输入\"退出\"或\"tz\"可切换回计算模式"

#define HELP_UNKNOWN_MODE     L"未知模式帮助"

#else
// ===========================================
// 英文界面配置 - English Interface Configuration
// ===========================================

// 快捷项名称 - Shortcut Names
#define ITEM_DOCUMENTS         L"Documents"
#define ITEM_PICTURES          L"Pictures"
#define ITEM_MUSIC             L"Music"
#define ITEM_VIDEOS            L"Videos"
#define ITEM_RECYCLE_BIN       L"Recycle Bin"
#define ITEM_CONTROL_PANEL     L"Control Panel"
#define ITEM_CALCULATOR        L"Calculator"
#define ITEM_NOTEPAD           L"Notepad"
#define ITEM_PAINT             L"Paint"
#define ITEM_CMD               L"Command Prompt"
#define ITEM_POWERSHELL        L"PowerShell"

// 搜索结果显示 - Search Results Display
#define SEARCH_PREFIX_DIR      L"DIR: "
#define SEARCH_PREFIX_URL      L"URL: "
#define SEARCH_PREFIX_APP      L"APP: "
#define SEARCH_NO_RESULTS      L"No matching results found"
#define SEARCH_COLLECTION      L"Collection:"

// 按钮文本 - Button Text
#define BTN_EXIT_CALC          L"Exit Calc"
#define BTN_EXIT               L"Exit"
#define BTN_SETTINGS           L"Settings"
#define BTN_EXIT_BOOKMARK      L"Exit"
#define LABEL_CALC             L"Calc:(Input q to exit)"
#define LABEL_SEARCH           L"Search:"
#define LABEL_URL              L"URL:"
#define MSG_INVALID_URL        L"Please enter a valid URL"
#define MSG_URL_EXISTS         L"This URL already exists in bookmarks"
#define MSG_ADD_BOOKMARK_FAIL  L"Add URL failed"
#define MSG_CLEAR_HISTORY      L"Are you sure you want to clear all calculation history?"
#define MSG_CALC_FAILED        L"Cannot calculate expression"
#define MSG_CALC_EXCEPTION     L"Expression calculation exception"
#define MENU_EXIT_CALC         L"Exit Calculator Mode"
#define MENU_EXIT_PROGRAM      L"Exit Program"
#define MSG_OPTIONS_IN_DEVELOPMENT L"Options feature is under development..."
#define DLG_HELP_TITLE         L"Help"
#define DLG_CALC_ERROR         L"Calculation Error"
#define BTN_ADD                L"Add"
#define BTN_UPDATE             L"Update"
#define BTN_DELETE             L"Delete"
#define BTN_CLOSE              L"Close"
#define BTN_SHOW_WINDOW        L"Show Window"

// 标签文本 - Label Text
#define LABEL_SEARCH           L"Search:"
#define LABEL_CALC             L"Calc:(Input q to exit)"
#define LABEL_URL              L"URL:"
#define LABEL_NAME             L"Name:"
#define LABEL_FAST_LAUNCH      L"Quick Launch"

// 对话框标题 - Dialog Titles
#define DLG_BOOKMARK_MANAGE    L"Bookmark Management"
#define DLG_ADD_FAILED         L"Add URL Failed"
#define DLG_DELETE_CONFIRM     L"Confirm Delete"
#define DLG_SYNC_FAILED        L"Synchronization Failed"
#define DLG_SYNC_SUCCESS       L"Synchronization Complete"
#define DLG_CALC_ERROR         L"Calculation Error"
#define DLG_HELP_INFO          L"Help Information"
#define DLG_DEVELOPING         L"Notice"

// 错误信息 - Error Messages
#define MSG_INVALID_URL        L"Please enter a valid URL"
#define MSG_URL_EXISTS         L"This URL already exists in bookmarks"
#define MSG_CHROME_NOT_FOUND   L"Chrome bookmarks file not found. Please ensure Chrome is installed and has at least one bookmark"
#define MSG_CHROME_READ_ERROR  L"Cannot read Chrome bookmarks file. Please ensure Chrome is closed"
#define MSG_USER_DATA_ERROR    L"Cannot get user data directory"
#define MSG_CALC_FAILED        L"Cannot calculate expression"
#define MSG_CALC_EXCEPTION     L"Expression calculation exception"
#define MSG_INPUT_EMPTY        L"Please enter name and URL"
#define MSG_INVALID_INPUT_URL  L"Please enter a valid URL"
#define MSG_SELECT_UPDATE      L"Please select a URL to update"
#define MSG_SELECT_DELETE      L"Please select a URL to delete"
#define MSG_DELETE_CONFIRM_TEXT L"Are you sure you want to delete the selected URL?"
#define MSG_CLEAR_HISTORY      L"Are you sure you want to clear all calculation history?"
#define MSG_ADD_URL_HINT       L"Please enter a valid URL"
#define MSG_SYNC_INFO          L"Successfully synchronized %d new bookmarks from Chrome"

// 设置菜单项 - Settings Menu Items
#define MENU_BOOKMARK_MANAGE   L"Bookmark Management..."
#define MENU_OPTIONS           L"Options..."
#define MENU_HELP              L"Help..."
#define MENU_EXIT_CALC         L"Exit Calculator Mode"
#define MENU_EXIT_PROGRAM      L"Exit Program"

// 上下文菜单项 - Context Menu Items
#define CONTEXT_DELETE_ITEM    L"Delete Item"
#define CONTEXT_CLEAR_ALL      L"Clear All History"
#define CONTEXT_DELETE_BOOKMARK L"Delete Bookmark"
#define CONTEXT_SYNC_CHROME    L"Synchronize Chrome Bookmarks"

// 帮助内容 - Help Content
#define HELP_CALCULATOR_TITLE  L"Calculator Mode Help\n\n"
#define HELP_CALCULATOR_BASIC  L"Basic Operations:\n• Enter expressions to calculate, e.g.: 2+3*4\n• Support arithmetic: + - * /\n• Support parentheses: () [] {}\n• Support decimals: 3.14\n\n"
#define HELP_CALCULATOR_ADVANCED L"Advanced Features:\n• Add comments: Enter expression # comment\n  Example: 1+2 #buy bread\n  Example: 10*5 #buy fruit\n• Comments are automatically right-aligned\n\n"
#define HELP_CALCULATOR_OPERATIONS L"Operations:\n• Enter expressions in the input box and press Enter to calculate\n• Calculation history is displayed in the list\n• Maximum 50 history records saved\n• Click history record to copy to input box\n• Enter \"js\" to switch to bookmark mode\n• Enter \"q\" to exit calculator mode"

#define HELP_BOOKMARK_TITLE   L"Bookmark Mode Help\n\n"
#define HELP_BOOKMARK_BASIC   L"Basic Operations:\n• Bookmark URLs: Enter \"name=URL\" to save\n  Example: Google=https://www.google.com\n• Open URLs: Double-click URL item in list\n• Delete URLs: Select URL and press Delete key\n• Edit URLs: Double-click URL to edit\n\n"
#define HELP_BOOKMARK_SHORTCUTS L"Quick Operations:\n• Right-click URLs for context menu\n• Support copy URL to clipboard\n• Support import/export bookmarks\n• Support batch operations\n\n"
#define HELP_BOOKMARK_FORMAT  L"Input Format:\n• Format: name=URL\n• No spaces around equals sign\n• URL must contain http:// or https://\n\n"
#define HELP_BOOKMARK_OPERATIONS L"Operations:\n• Enter URL information in input box\n• Calculation history records operations\n• Enter \"exit\" or \"tz\" to switch back to calculator mode"

#define HELP_UNKNOWN_MODE     L"Unknown mode help"

#endif

#endif // INTERNATIONALIZATION_H
#pragma once

#ifndef BOOKMARK_MANAGER_H
#define BOOKMARK_MANAGER_H

#define _CRT_SECURE_NO_WARNINGS 1
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <string>
#include <vector>

/**
 * @brief 网址管理模块头文件
 * 
 * 此文件包含所有网址收藏管理相关的函数声明，
 * 包括添加、删除、保存、加载、搜索和同步Chrome书签等功能。
 */

// 网址管理相关函数声明

/**
 * @brief 添加网址收藏
 * 
 * 此函数用于添加新的网址收藏到收藏列表
 * 
 * @param name 网址名称
 * @param url 网址URL
 */
void AddBookmark(const WCHAR* name, const WCHAR* url);

/**
 * @brief 删除网址收藏
 * 
 * 此函数用于从收藏列表中删除指定索引的网址收藏
 * 
 * @param index 要删除的网址收藏索引
 */
void DeleteBookmark(int index);

/**
 * @brief 保存网址收藏到文件
 * 
 * 此函数将当前网址收藏列表保存到data\bookmarks.txt文件中
 */
void SaveBookmarks();

/**
 * @brief 从文件加载网址收藏
 * 
 * 此函数从data\bookmarks.txt文件中加载网址收藏列表
 */
void LoadBookmarks();

/**
 * @brief 同步Chrome书签
 * 
 * 此函数从Chrome浏览器中同步书签到本地网址收藏列表
 */
void SyncChromeBookmarks();

/**
 * @brief 搜索网址收藏
 * 
 * 此函数根据查询字符串搜索网址收藏，支持名称和URL的模糊搜索
 * 
 * @param query 搜索查询字符串
 */
void SearchBookmarks(const WCHAR* query);

/**
 * @brief 显示网址管理对话框
 * 
 * 此函数显示网址收藏管理对话框，允许用户添加、编辑和删除网址收藏
 */
void ShowBookmarkDialog();

/**
 * @brief 刷新网址列表显示
 * 
 * 此函数用于刷新列表框中的网址收藏显示
 */
void DisplayBookmarkResults();

/**
 * @brief 检查字符串是否为有效的URL
 * 
 * 此函数检查给定的字符串是否符合URL格式
 * 
 * @param text 要检查的字符串
 * @return true 如果是有效的URL
 * @return false 如果不是有效的URL
 */
bool IsURL(const WCHAR* text);

/**
 * @brief 网址管理对话框过程
 * 
 * 此函数处理网址管理对话框的消息处理
 * 
 * @param hwnd 对话框窗口句柄
 * @param uMsg 消息类型
 * @param wParam 消息参数
 * @param lParam 消息参数
 * @return INT_PTR 消息处理结果
 */
INT_PTR CALLBACK BookmarkDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**
 * @brief 刷新对话框中的网址列表
 * 
 * 此函数刷新对话框中的网址列表显示
 * 
 * @param hList 列表框句柄
 */
void RefreshBookmarkList(HWND hList);

/**
 * @brief 从对话框添加网址
 * 
 * 此函数处理从对话框添加网址的操作
 * 
 * @param hDlg 对话框句柄
 */
void AddBookmarkFromDialog(HWND hDlg);

/**
 * @brief 从对话框更新网址
 * 
 * 此函数处理从对话框更新网址的操作
 * 
 * @param hDlg 对话框句柄
 */
void UpdateBookmarkFromDialog(HWND hDlg);

/**
 * @brief 从对话框删除网址
 * 
 * 此函数处理从对话框删除网址的操作
 * 
 * @param hDlg 对话框句柄
 */
void DeleteBookmarkFromDialog(HWND hDlg);

#endif // BOOKMARK_MANAGER_H
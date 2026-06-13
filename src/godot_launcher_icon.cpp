#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <ui/ui.h>

#include "godot_launcher_icon.h"

namespace {

using Microsoft::WRL::ComPtr;

/**
 * @brief 确保当前线程只初始化一次 COM。
 */
void EnsureComInitialized()
{
    static std::once_flag once;
    std::call_once(once, []() {
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    });
}

/**
 * @brief 获取全局 WIC 工厂实例。
 * @return ComPtr<IWICImagingFactory> WIC 工厂
 */
ComPtr<IWICImagingFactory> GetWicFactory()
{
    EnsureComInitialized();
    static std::once_flag once;
    static ComPtr<IWICImagingFactory> factory;
    std::call_once(once, []() {
        CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    });
    return factory;
}

/**
 * @brief 把图标位置字符串拆成“文件路径 + 图标索引”。
 * @param iconLocation 原始图标位置
 * @param outPath 输出路径
 * @param outIndex 输出索引
 * @return true 解析成功
 */
bool ParseIconLocation(const std::wstring& iconLocation, std::wstring& outPath, int& outIndex)
{
    size_t comma = iconLocation.find_last_of(L',');
    if (comma == std::wstring::npos)
    {
        outPath = iconLocation;
        outIndex = 0;
        return !outPath.empty();
    }

    outPath = iconLocation.substr(0, comma);
    std::wstring indexText = iconLocation.substr(comma + 1);
    outIndex = _wtoi(indexText.c_str());
    return !outPath.empty();
}

/**
 * @brief 将 HICON 编码为 PNG 二进制字节。
 * @param hIcon 图标句柄
 * @return std::vector<std::uint8_t> PNG 数据，失败返回空
 */
std::vector<std::uint8_t> EncodeIconToPngBytes(HICON hIcon)
{
    std::vector<std::uint8_t> png;
    if (!hIcon)
    {
        return png;
    }

    ComPtr<IWICImagingFactory> factory = GetWicFactory();
    if (!factory)
    {
        return png;
    }

    ComPtr<IWICBitmap> wicBitmap;
    if (FAILED(factory->CreateBitmapFromHICON(hIcon, &wicBitmap)) || !wicBitmap)
    {
        return png;
    }

    ComPtr<IStream> stream;
    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &stream)) || !stream)
    {
        return png;
    }

    ComPtr<IWICBitmapEncoder> encoder;
    if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, NULL, &encoder)) || !encoder)
    {
        return png;
    }
    if (FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)))
    {
        return png;
    }

    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> props;
    if (FAILED(encoder->CreateNewFrame(&frame, &props)) || !frame)
    {
        return png;
    }
    if (FAILED(frame->Initialize(props.Get())))
    {
        return png;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(wicBitmap->GetSize(&width, &height)) || width == 0 || height == 0)
    {
        return png;
    }
    if (FAILED(frame->SetSize(width, height)))
    {
        return png;
    }

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
    if (FAILED(frame->SetPixelFormat(&format)))
    {
        return png;
    }
    if (FAILED(frame->WriteSource(wicBitmap.Get(), NULL)))
    {
        return png;
    }
    if (FAILED(frame->Commit()) || FAILED(encoder->Commit()))
    {
        return png;
    }

    HGLOBAL hGlobal = NULL;
    if (FAILED(GetHGlobalFromStream(stream.Get(), &hGlobal)) || !hGlobal)
    {
        return png;
    }

    SIZE_T sizeBytes = GlobalSize(hGlobal);
    if (sizeBytes == 0)
    {
        return png;
    }

    void* mem = GlobalLock(hGlobal);
    if (!mem)
    {
        return png;
    }

    png.resize(sizeBytes);
    memcpy(png.data(), mem, sizeBytes);
    GlobalUnlock(hGlobal);
    return png;
}

/**
 * @brief 从 shell 路径、普通路径或 iconLocation 中提取 HICON。
 * @param source 图标来源
 * @param size 期望尺寸
 * @return HICON 图标句柄，调用方负责销毁
 */
HICON ExtractIconHandle(const std::wstring& source, int size)
{
    if (source.empty())
    {
        return NULL;
    }

    SHFILEINFOW sfi = {0};
    UINT flags = SHGFI_ICON | (size > 24 ? SHGFI_LARGEICON : SHGFI_SMALLICON);

    if (source.rfind(L"shell:", 0) == 0)
    {
        PIDLIST_ABSOLUTE pidl = NULL;
        if (SUCCEEDED(SHParseDisplayName(source.c_str(), NULL, &pidl, 0, NULL)) && pidl)
        {
            if (SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), 0, &sfi, sizeof(sfi), flags | SHGFI_PIDL))
            {
                CoTaskMemFree(pidl);
                return sfi.hIcon;
            }
            CoTaskMemFree(pidl);
        }
    }

    std::wstring iconPath;
    int iconIndex = 0;
    if (ParseIconLocation(source, iconPath, iconIndex))
    {
        const DWORD attrs = GetFileAttributesW(iconPath.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            HICON largeIcon = NULL;
            HICON smallIcon = NULL;
            UINT extracted = ExtractIconExW(iconPath.c_str(), iconIndex, &largeIcon, &smallIcon, 1);
            if (extracted > 0)
            {
                if (size > 24 && largeIcon)
                {
                    if (smallIcon) DestroyIcon(smallIcon);
                    return largeIcon;
                }
                if (smallIcon)
                {
                    if (largeIcon) DestroyIcon(largeIcon);
                    return smallIcon;
                }
                if (largeIcon)
                {
                    return largeIcon;
                }
            }
        }
    }

    if (SHGetFileInfoW(source.c_str(), 0, &sfi, sizeof(sfi), flags))
    {
        return sfi.hIcon;
    }

    return NULL;
}

/**
 * @brief 根据快捷方式推导最合适的图标来源路径。
 * @param shortcut 快捷方式
 * @return std::wstring 图标来源
 */
std::wstring GetBestIconSource(const ShortcutItem& shortcut)
{
    if (wcslen(shortcut.iconPath) > 0 && wcsncmp(shortcut.iconPath, L"emoji:", 6) != 0)
    {
        return shortcut.iconPath;
    }
    if (wcslen(shortcut.path) > 0)
    {
        return shortcut.path;
    }

    if (shortcut.type == 0)
    {
        return L"shell32.dll,4";
    }
    return L"shell32.dll,2";
}

} // namespace

/**
 * @brief 获取快捷方式对应的真实图标，并做内存缓存复用。
 * @param shortcut 快捷方式对象
 * @param size 期望图标尺寸
 * @return std::shared_ptr<ui::ImageHandle> 图标句柄，失败返回空
 */
std::shared_ptr<ui::ImageHandle> GodotGetShortcutIconImage(const ShortcutItem& shortcut, int size)
{
    if (size <= 0)
    {
        return nullptr;
    }

    static std::mutex cacheMutex;
    static std::unordered_map<std::wstring, std::shared_ptr<ui::ImageHandle>> cache;

    std::wstring source = GetBestIconSource(shortcut);
    if (source.empty())
    {
        return nullptr;
    }

    std::wstring cacheKey = source + L"#" + std::to_wstring(size);
    {
        std::lock_guard<std::mutex> guard(cacheMutex);
        auto it = cache.find(cacheKey);
        if (it != cache.end())
        {
            return it->second;
        }
    }

    HICON hIcon = ExtractIconHandle(source, size);
    if (!hIcon)
    {
        return nullptr;
    }

    std::vector<std::uint8_t> png = EncodeIconToPngBytes(hIcon);
    DestroyIcon(hIcon);
    if (png.empty())
    {
        return nullptr;
    }

    std::shared_ptr<ui::ImageHandle> image = ui::ImageHandle::from_encoded_bytes(std::move(png), source);
    {
        std::lock_guard<std::mutex> guard(cacheMutex);
        cache[cacheKey] = image;
    }
    return image;
}

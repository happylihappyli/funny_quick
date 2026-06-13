import os
import subprocess
import datetime

# 导入SCons环境
from SCons.Environment import Environment

# 创建基本环境，确保包含所有必要的工具
env = Environment(
    tools=['clang', 'clang++', 'ar', 'link'],
    tool_path=['D:/scoop/apps/llvm/current/bin']
)

# 编译开始时间
start_time = datetime.datetime.now()

def play_compilation_tts(message="任务运行完毕，过来看看！"):
    """
    播放编译完成TTS语音提示
    """
    try:
        # 调用Python TTS脚本
        subprocess.run([
            'python', 'tts_notification.py', message
        ], check=False, shell=True)
        print(f"TTS播放: {message}")
    except Exception as e:
        print(f"TTS播放失败: {e}")

def show_compilation_time(message="编译开始"):
    """
    显示编译时间信息
    """
    current_time = datetime.datetime.now()
    print(f"{message}时间: {current_time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    if message == "编译结束":
        duration = current_time - start_time
        print(f"编译耗时: {duration.total_seconds():.2f}秒")
        
        # 播放编译完成的TTS提示
        play_compilation_tts("编译已完成，任务运行完毕，可以测试 快捷方式管理器！")
    elif message == "编译开始":
        # 播放编译开始的TTS提示
        play_compilation_tts("开始编译项目")
        print("=" * 50)

# 显示编译开始时间
show_compilation_time("编译开始")

# 在Windows环境下，构建前终止正在运行的funny_quick进程
def kill_running_processes():
    if os.name == 'nt':
        try:
            # 使用PowerShell命令终止所有 funny_quick 相关进程
            subprocess.run([
                'powershell.exe', 
                '-Command', 
                'Get-Process funny_quick,funny_quick_webview -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue'
            ], check=False, shell=True)
            print("已终止所有正在运行的 funny_quick 相关进程")
        except Exception as e:
            print(f"尝试终止进程时出错: {str(e)}")

# 确保bin目录存在
def ensure_bin_directory():
    bin_dir = 'bin'
    if not os.path.exists(bin_dir):
        os.makedirs(bin_dir)
        print(f"已创建bin目录: {os.path.abspath(bin_dir)}")
    return bin_dir

# 确保obj目录存在
def ensure_obj_directory():
    obj_dir = 'obj'
    if not os.path.exists(obj_dir):
        os.makedirs(obj_dir)
        print(f"已创建obj目录: {os.path.abspath(obj_dir)}")
    return obj_dir

# Windows环境特殊设置
if os.name == 'nt':
    # WebView2 路径配置
    webview2_sdk_path = os.path.abspath(r'packages\microsoft.web.webview2\1.0.3405.78')
    webview2_include = os.path.join(webview2_sdk_path, 'build', 'native', 'include')
    webview2_lib_x64 = os.path.join(webview2_sdk_path, 'build', 'native', 'x64')
    godot_ui_root = os.path.abspath(r'..\godot_ui\godot-ui-standalone-direct2d')
    godot_ui_include = os.path.join(godot_ui_root, 'include')

    # Clang 环境特殊设置
    env.Append(CXXFLAGS=['-std=c++2b', '-Wall', '-Wextra', '-g', '-D_CRT_SECURE_NO_WARNINGS', '-DNOMINMAX', '-DIDI_APP_ICON=1001'])
    env.Append(LINKFLAGS=['-mwindows'])
    env.Append(CPPPATH=['src', '.', webview2_include, godot_ui_include])
    env.Append(LIBPATH=[webview2_lib_x64])
    env.Append(LIBS=['user32', 'gdi32', 'comctl32', 'shell32', 'advapi32', 'ole32', 'oleaut32', 'uuid', 'imm32', 'comdlg32', 'windowscodecs', 'd2d1', 'dwrite', 'dxguid', 'dbghelp', 'WebView2LoaderStatic'])
    print("Clang 编译器配置成功")

# WebView 旧版源文件
webview_sources = ['src/gui_main.cpp', 'src/command_handler.cpp', 'src/logger.cpp', 'src/webview_manager.cpp', 'src/dir_mode_manager.cpp', 'src/window_size_handler.cpp', 'src/message_handlers.cpp', 'src/calculator.cpp', 'src/bookmark_manager.cpp', 'src/file_manager.cpp', 'src/file_search_manager.cpp', 'src/ui_helpers.cpp', 'src/tray_icon_manager.cpp', 'src/command_processor.cpp']

# Godot UI 新版源文件
godot_core_sources = [
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_layout_controls.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_editor_controls.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_editor_view_model.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_menu_controls.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_scene_canvas.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_scroll_list_tree.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_text_controls.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_visual_controls.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'ui_window_controls.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'backend_win32_d2d.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_document.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_build_tree.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_common.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_parse.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_props_controls.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_shared_parsers.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_specific_dispatch.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_type_routing.cpp'),
    os.path.join('..', 'godot_ui', 'godot-ui-standalone-direct2d', 'src', 'tscn_loader_visual_window.cpp'),
]
godot_sources = ['src/godot_ui_main.cpp', 'src/godot_launcher_backend.cpp', 'src/godot_launcher_icon.cpp', 'src/logger.cpp'] + godot_core_sources

# 资源文件
if os.name == 'nt':
    # 尝试查找资源编译器
    try:
        # 优先使用 llvm-rc
        env['RC'] = 'llvm-rc'
        # 尝试添加代码页参数 /c 65001
        env['RCFLAGS'] = ['/c', '65001'] 
        
        # 定义资源文件构建规则
        # 注意: /FO 参数必须紧跟输出文件名，所以在 action 中显式指定
        rc_builder = Builder(action='$RC $RCFLAGS /FO $TARGET $SOURCE',
                             suffix='.res',
                             src_suffix='.rc')
        env.Append(BUILDERS={'Resource': rc_builder})
        
        # 这里只添加文件名，后续循环会处理构建
        resource_files = ['resource.rc']
        print("已添加资源文件构建 (使用 llvm-rc)")
    except Exception as e:
        print(f"资源构建配置出错: {e}")
        resource_files = []
else:
    resource_files = []

# 在构建前执行预处理任务
bin_dir = ensure_bin_directory()
obj_dir = ensure_obj_directory()
kill_running_processes()

# 设置对象文件输出目录
env['OBJDIR'] = obj_dir
# 为每个源文件设置输出路径
env['OBJSUFFIX'] = '.obj'
env['PROGSUFFIX'] = '.exe'

# 构建对象文件工具函数
def build_objects(source_list):
    objects = []
    for src in source_list:
        base_name = os.path.splitext(os.path.basename(src))[0]
        obj_name = os.path.join(obj_dir, base_name + '.obj')
        obj = env.Object(target=obj_name, source=src)
        objects.append(obj)
    return objects

# 为每个资源文件创建资源文件节点
resource_objects = []
for rc in resource_files:
    # 获取文件名（不含扩展名）
    base_name = os.path.splitext(os.path.basename(rc))[0]
    # 创建资源文件路径
    res_name = os.path.join(obj_dir, base_name + '.res')
    # 构建资源文件 - 使用自定义的Resource构建器
    res = env.Resource(target=res_name, source=rc)
    resource_objects.append(res)

# 使用对象文件和资源文件构建两个可执行文件
webview_target_path = os.path.join(bin_dir, 'funny_quick_webview')
godot_target_path = os.path.join(bin_dir, 'funny_quick')

webview_objects = build_objects(webview_sources)
godot_objects = build_objects(godot_sources)

webview_executable = env.Program(target=webview_target_path, source=webview_objects + resource_objects)
godot_executable = env.Program(target=godot_target_path, source=godot_objects + resource_objects)

# 复制图标文件到bin目录
if os.path.exists('app_icon.ico'):
    icon_target = os.path.join(bin_dir, 'app_icon.ico')
    # 使用env.Command创建复制任务
    icon_copy = env.Command(icon_target, 'app_icon.ico', Copy('$TARGET', '$SOURCE'))
    # 确保图标复制在构建时执行
    env.Depends(webview_executable, icon_copy)
    env.Depends(godot_executable, icon_copy)
    print("将复制图标文件到bin目录")
else:
    print("警告：未找到app_icon.ico文件")

# 复制HTML模板文件到bin/data目录
def copy_html_templates():
    """复制HTML模板文件到bin/data目录"""
    data_dir = os.path.join(bin_dir, 'data')
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)
        print(f"已创建data目录: {os.path.abspath(data_dir)}")
    
    # 复制dir_mode_template.html
    html_files = ['dir_mode_template.html', 'webview_template.html', 'shortcut_edit_template.html', 'calculator_template.html']
    for html_file in html_files:
        html_source = os.path.join('data', html_file)
        if os.path.exists(html_source):
            html_target = os.path.join(data_dir, html_file)
            # 使用env.Command创建复制任务
            html_copy = env.Command(html_target, html_source, Copy('$TARGET', '$SOURCE'))
            # 确保HTML复制在构建时执行
            env.Depends(webview_executable, html_copy)
            print(f"将复制 {html_file} 到bin/data目录")
        else:
            print(f"警告：未找到HTML模板文件: {html_source}")

# 执行HTML模板复制
copy_html_templates()

# 设置清理目标
Clean(webview_executable, os.path.join(bin_dir, 'funny_quick_webview.exe'))
Clean(godot_executable, os.path.join(bin_dir, 'funny_quick.exe'))
# 清理obj目录中的所有对象文件
for src in webview_sources + godot_sources:
    base_name = os.path.splitext(os.path.basename(src))[0]
    obj_name = os.path.join(obj_dir, base_name + '.obj')
    Clean(webview_executable, obj_name)
    Clean(godot_executable, obj_name)
# 清理复制的图标文件
if os.path.exists('app_icon.ico'):
    Clean(webview_executable, os.path.join(bin_dir, 'app_icon.ico'))
    Clean(godot_executable, os.path.join(bin_dir, 'app_icon.ico'))

Default([godot_executable, webview_executable])

print("使用 'scons' 构建项目")
print("使用 'scons -c' 清理项目")

# 设置编译完成后的钩子
import atexit
import signal
import sys

def cleanup_callback():
    """清理回调函数，在程序退出时显示编译结束时间"""
    show_compilation_time("编译结束")

# 注册多个退出事件以确保时间显示
atexit.register(cleanup_callback)
signal.signal(signal.SIGINT, lambda s, f: (cleanup_callback(), sys.exit(0)))

# 显示分隔线
print("=" * 50)

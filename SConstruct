import os
import subprocess
import datetime

# 创建基本环境，确保包含所有必要的工具
env = Environment(tools=['default'])

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
        play_compilation_tts("编译已完成，任务运行完毕，过来看看！")
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
            # 使用PowerShell命令终止所有funny_quick进程
            subprocess.run([
                'powershell.exe', 
                '-Command', 
                'Get-Process funny_quick -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue'
            ], check=False, shell=True)
            print("已终止所有正在运行的funny_quick进程")
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
    # 添加Windows特定库（使用Clang风格的库名称）
    # 注意：这些库将在Clang配置部分被清除并重新添加
    
    # WebView2 路径配置
    webview2_sdk_path = r'C:\Users\happyli\.nuget\packages\microsoft.web.webview2\1.0.3405.78'
    webview2_include = os.path.join(webview2_sdk_path, 'build', 'native', 'include')
    webview2_lib_x64 = os.path.join(webview2_sdk_path, 'build', 'native', 'x64')
    webview2_lib_x86 = os.path.join(webview2_sdk_path, 'build', 'native', 'x86')
    
    # 添加 WebView2 头文件路径
    if os.path.exists(webview2_include):
        env.Append(CPPPATH=[webview2_include])
        print(f"WebView2 头文件路径: {webview2_include}")
    else:
        print(f"警告: WebView2 头文件路径不存在: {webview2_include}")
    
    # 添加 WebView2 库文件路径（优先使用 x64）
    # 注意：这些库路径将在Clang配置部分被清除并重新添加
    if os.path.exists(webview2_lib_x64):
        print(f"WebView2 库文件路径 (x64): {webview2_lib_x64}")
    elif os.path.exists(webview2_lib_x86):
        print(f"WebView2 库文件路径 (x86): {webview2_lib_x86}")
    else:
        print(f"警告: WebView2 库文件路径不存在")
    
    # 添加资源文件生成支持
    # 由于rc.exe不可用，使用Python脚本生成资源文件
    print("使用Python脚本生成资源文件（rc.exe不可用）")
    
    # 创建自定义的资源文件构建器
    def generate_resource(target, source, env):
        """使用Python脚本生成资源文件"""
        import subprocess
        try:
            # 运行Python脚本生成资源文件
            result = subprocess.run([
                'python', 'test/generate_resource.py'
            ], capture_output=True, text=True, cwd=env.Dir('.').abspath)
            
            if result.returncode == 0:
                print("资源文件生成成功")
                return None
            else:
                print(f"资源文件生成失败: {result.stderr}")
                return "资源文件生成失败"
        except Exception as e:
            print(f"运行资源生成脚本时出错: {e}")
            return f"脚本执行错误: {e}"
    
    # 添加自定义构建器
    resource_builder = Builder(action=generate_resource)
    env.Append(BUILDERS={'Resource': resource_builder})
    
    # 设置资源文件构建规则
    env['RESOURCE_TARGET'] = 'obj/resource.res'
    env['RESOURCE_SOURCE'] = 'resource.rc'
    env['RCFLAGS'] = '/c65001'  # 设置UTF-8编码
    env['BUILDERS']['RES'] = Builder(action='$RCCOM', suffix='.res', src_suffix='.rc')
    
    # 使用Windows API版本
    print("使用Windows API版本")
    
        # 强制使用Clang编译器
    try:
        # 首先检测Clang版本是否兼容
        import subprocess
        clang_version_ok = False
        try:
            # 获取Clang版本信息
            result = subprocess.run(['clang', '--version'], capture_output=True, text=True, shell=True)
            if result.returncode == 0:
                version_output = result.stdout
                # 提取版本号
                import re
                version_match = re.search(r'clang version (\d+)\.(\d+)\.(\d+)', version_output)
                if version_match:
                    major_version = int(version_match.group(1))
                    if major_version >= 19:
                        print(f"检测到兼容的Clang编译器版本: {major_version}")
                        clang_version_ok = True
                    else:
                        print(f"警告：检测到Clang版本 {major_version}，需要Clang 19.0.0或更高版本")
                        print("将使用Visual Studio编译器")
                        clang_version_ok = False
                else:
                    print("无法解析Clang版本信息，将使用Visual Studio编译器")
                    clang_version_ok = False
            else:
                print("无法获取Clang版本信息，将使用Visual Studio编译器")
                clang_version_ok = False
        except Exception as e:
            print(f"Clang版本检测失败: {e}")
            clang_version_ok = False
        
        # 如果Clang版本兼容，则使用Clang编译器
        if clang_version_ok:
            # 首先尝试使用Clang编译器
            env.Tool('clang')
            # 设置Clang特定的编译标志
            env['CXXFLAGS'] = ['-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-deprecated-declarations', '-fms-extensions', '-fms-compatibility']
            # 设置Clang特定的编译命令格式 - 使用GCC风格的include路径
            env['CXXCOM'] = '$CXX -o $TARGET -c $CXXFLAGS $_CPPDEFFLAGS -I$CPPPATH $SOURCES'
            # 添加src目录到头文件搜索路径
            env.Append(CPPPATH=['src', '.'])
            print("使用Clang编译器")
        else:
            # 如果Clang版本不兼容，强制使用Visual Studio编译器
            raise Exception("Clang版本不兼容，强制使用Visual Studio编译器")
    except:
        try:
            # 如果Clang不可用，尝试手动配置Clang
            # 设置Clang编译器路径
            clang_paths = [
                r"C:\Program Files\LLVM\bin",
                r"C:\Program Files (x86)\LLVM\bin",
                r"D:\Code\LLVM\bin"
            ]
            
            clang_found = False
            clang_version_ok = False
            
            for clang_path in clang_paths:
                if os.path.exists(clang_path):
                    # 检测Clang版本是否兼容
                    try:
                        # 获取Clang版本信息
                        clang_exe = os.path.join(clang_path, 'clang.exe')
                        result = subprocess.run([clang_exe, '--version'], capture_output=True, text=True, shell=True)
                        if result.returncode == 0:
                            version_output = result.stdout
                            # 提取版本号
                            import re
                            version_match = re.search(r'clang version (\d+)\.(\d+)\.(\d+)', version_output)
                            if version_match:
                                major_version = int(version_match.group(1))
                                if major_version >= 19:
                                    print(f"检测到兼容的Clang编译器版本: {major_version}")
                                    clang_version_ok = True
                                else:
                                    print(f"警告：检测到Clang版本 {major_version}，需要Clang 19.0.0或更高版本")
                                    print("将跳过此路径，继续查找其他编译器")
                                    continue
                            else:
                                print("无法解析Clang版本信息，将跳过此路径")
                                continue
                        else:
                            print("无法获取Clang版本信息，将跳过此路径")
                            continue
                    except Exception as e:
                        print(f"Clang版本检测失败: {e}")
                        continue
                    
                    # 如果Clang版本兼容，则配置编译器
                    if clang_version_ok:
                         # 设置编译器路径
                         env['ENV']['PATH'] = clang_path + ";" + os.environ.get('PATH', '')
                         env['CXX'] = 'clang++.exe'
                         # 设置Clang特定的编译标志
                         env['CXXFLAGS'] = ['-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-deprecated-declarations', '-fms-extensions', '-fms-compatibility', '-Wno-error', '-Wno-enum-constexpr-conversion', '-Wno-bitfield-enum-conversion', '-Wno-enum-compare', '-Wno-enum-conversion']
                         # 设置Clang特定的编译命令格式 - 使用GCC风格的include路径
                         env['CXXCOM'] = '$CXX -o $TARGET -c $CXXFLAGS $_CPPDEFFLAGS -I$CPPPATH $SOURCES'
                         # 添加src目录到头文件搜索路径
                         env.Append(CPPPATH=['src', '.'])
                         
                         # 清除旧的库配置，使用Clang风格的配置
                         env['LIBS'] = []
                         env['LIBPATH'] = []
                         
                         print(f"手动配置Clang编译器成功: {clang_path}")
                         clang_found = True
                         break
            
            if not clang_found:
                # 如果Clang不可用，回退到Visual Studio编译器
                env.Tool('msvc')
                env['CXXFLAGS'] = ['/EHsc', '/W3', '/utf-8']
                # 添加src目录到头文件搜索路径
                env.Append(CPPPATH=['src', '.'])
                print("Clang不可用，使用Visual Studio编译器")
                
        except:
            # 如果都不可用，尝试手动设置Visual Studio环境
            try:
                # 设置Visual Studio 2022路径
                vs_path = r"D:\Code\VS2022\Community"
                vc_path = os.path.join(vs_path, "VC", "Tools", "MSVC", "14.44.35207")
                
                if os.path.exists(vc_path):
                    # 设置编译器路径
                    env['ENV']['PATH'] = os.path.join(vc_path, "bin", "Hostx64", "x64") + ";" + os.environ.get('PATH', '')
                    env['ENV']['INCLUDE'] = os.path.join(vc_path, "include") + ";" + os.environ.get('INCLUDE', '')
                    env['ENV']['LIB'] = os.path.join(vc_path, "lib", "x64") + ";" + os.environ.get('LIB', '')
                    
                    # 设置Windows SDK路径
                    windows_sdk_path = r"C:\Program Files (x86)\Windows Kits\10"
                    if os.path.exists(windows_sdk_path):
                        # 查找最新的Windows SDK版本
                        sdk_versions = []
                        for item in os.listdir(os.path.join(windows_sdk_path, "Include")):
                            if item.startswith("10."):
                                sdk_versions.append(item)
                        
                        if sdk_versions:
                            latest_sdk = max(sdk_versions)
                            include_path = os.path.join(windows_sdk_path, "Include", latest_sdk, "um")
                            lib_path = os.path.join(windows_sdk_path, "Lib", latest_sdk, "um", "x64")
                            
                            env['ENV']['INCLUDE'] += ";" + include_path
                            env['ENV']['LIB'] += ";" + lib_path
                    
                    env['CXX'] = 'cl.exe'
                    env['CXXFLAGS'] = ['/EHsc', '/W3', '/utf-8']
                    env.Append(CPPPATH=['src', '.'])
                    print("手动配置Visual Studio编译器成功")
                else:
                    raise Exception("Visual Studio路径不存在")
            except Exception as e:
                print(f"手动配置编译器失败: {e}")
                print("\n" + "="*60)
                print("错误：系统中未找到可用的C++编译器！")
                print("="*60)
                print("\n请安装以下编译器之一：")
                print("1. Clang编译器（推荐）：")
                print("   - 下载地址：https://github.com/llvm/llvm-project/releases")
                print("   - 安装到D:\\Code\\LLVM\\bin目录")
                print("2. Visual Studio 2022：")
                print("   - 下载地址：https://visualstudio.microsoft.com/")
                print("   - 安装到D:\\Code\\VS2022\\Community目录")
                print("3. MinGW-w64：")
                print("   - 下载地址：https://www.mingw-w64.org/")
                print("\n安装完成后，请重新运行scons命令。")
                print("="*60)
                # 退出构建过程
                Exit(1)
else:
    # 非Windows环境
    env['CXXFLAGS'] = ['-std=c++17', '-Wall', '-Wextra']

# 源文件 - 使用Windows API版本（文件已移动到src目录）
sources = ['src/gui_main.cpp', 'src/command_handler.cpp', 'src/logger.cpp', 'src/webview_manager.cpp', 'src/dir_mode_manager.cpp', 'src/window_size_handler.cpp', 'src/message_handlers.cpp', 'src/calculator.cpp', 'src/bookmark_manager.cpp', 'src/file_manager.cpp', 'src/file_search_manager.cpp']

# Windows环境下暂时不使用资源文件（resource.rc和resource.h在根目录）
# 因为资源文件生成脚本存在问题，暂时禁用资源文件构建
if os.name == 'nt':
    resource_files = []
    print("注意：资源文件构建已禁用，对话框将在代码中动态创建")
else:
    resource_files = []

# Windows GUI应用程序设置
if os.name == 'nt':
    # 使用Clang编译器，但添加特定的错误抑制选项
    if clang_found:
        print("使用Clang编译器，添加WebView2头文件兼容性选项")
        # 设置Clang编译器标志，添加更多错误抑制选项
        env['CXXFLAGS'] = ['-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-deprecated-declarations', 
                          '-fms-extensions', '-fms-compatibility', '-Wno-error',
                          '-Wno-bitfield-enum-conversion', '-Wno-enum-compare', '-Wno-enum-conversion',
                          '-Wno-invalid-source-encoding', '-Wno-c++11-narrowing', '-Wno-c99-extensions',
                          '-Wno-static-in-inline']
        # 设置Clang特定的编译命令格式 - 使用GCC风格的include路径
        env['CXXCOM'] = '$CXX -o $TARGET -c $CXXFLAGS $_CPPDEFFLAGS -I$CPPPATH $SOURCES'
        # 设置Clang特定的链接标志
        env['LINKCOM'] = '$CXX -o $TARGET $SOURCES $LINKFLAGS -L$LIBPATH $_LIBFLAGS'
        # 清除旧的链接器标志，使用Clang风格的参数
        env['LINKFLAGS'] = ['-Wl,--subsystem=windows', '-Wl,--entry=WinMainCRTStartup', '-static', '-lmsvcrt', '-lshell32', '-luser32', '-lgdi32', '-lcomctl32', '-limm32', '-ladvapi32', '-lole32', '-loleaut32', '-lWebView2LoaderStatic']
        # 设置库文件路径（使用Clang风格的-L参数）
        env['LIBPATH'] = [webview2_lib_x64]
    else:
        # 如果Clang不可用，使用Visual Studio编译器设置
        env['CXXFLAGS'] = ['/EHsc', '/W3', '/utf-8']

# 在构建前执行预处理任务
bin_dir = ensure_bin_directory()
obj_dir = ensure_obj_directory()
kill_running_processes()

# 设置对象文件输出目录
env['OBJDIR'] = obj_dir
# 为每个源文件设置输出路径
env['OBJSUFFIX'] = '.obj'
env['PROGSUFFIX'] = '.exe'

# 构建可执行文件到bin目录
target_path = os.path.join(bin_dir, 'funny_quick')

# 为每个源文件创建对象文件节点
object_files = []
for src in sources:
    # 获取文件名（不含扩展名）
    base_name = os.path.splitext(os.path.basename(src))[0]
    # 创建对象文件路径
    obj_name = os.path.join(obj_dir, base_name + '.obj')
    # 构建对象文件
    obj = env.Object(target=obj_name, source=src)
    object_files.append(obj)

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

# 使用对象文件和资源文件构建可执行文件
all_objects = object_files + resource_objects
executable = env.Program(target=target_path, source=all_objects)

# 复制图标文件到bin目录
if os.path.exists('app_icon.ico'):
    icon_target = os.path.join(bin_dir, 'app_icon.ico')
    # 使用env.Command创建复制任务
    icon_copy = env.Command(icon_target, 'app_icon.ico', Copy('$TARGET', '$SOURCE'))
    # 确保图标复制在构建时执行
    env.Depends(executable, icon_copy)
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
    html_source = 'data/dir_mode_template.html'
    if os.path.exists(html_source):
        html_target = os.path.join(data_dir, 'dir_mode_template.html')
        # 使用env.Command创建复制任务
        html_copy = env.Command(html_target, html_source, Copy('$TARGET', '$SOURCE'))
        # 确保HTML复制在构建时执行
        env.Depends(executable, html_copy)
        print("将复制HTML模板文件到bin/data目录")
    else:
        print(f"警告：未找到HTML模板文件: {html_source}")

# 执行HTML模板复制
copy_html_templates()

# 设置清理目标
Clean(executable, os.path.join(bin_dir, 'funny_quick.exe'))
# 清理obj目录中的所有对象文件
for src in sources:
    base_name = os.path.splitext(os.path.basename(src))[0]
    obj_name = os.path.join(obj_dir, base_name + '.obj')
    Clean(executable, obj_name)
# 清理复制的图标文件
if os.path.exists('app_icon.ico'):
    Clean(executable, os.path.join(bin_dir, 'app_icon.ico'))

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
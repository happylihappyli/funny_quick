import os
import subprocess

# 创建基本环境，确保包含所有必要的工具
env = Environment(tools=['default'])

# 在Windows环境下，构建前终止正在运行的quick_launcher进程
def kill_running_processes():
    if os.name == 'nt':
        try:
            # 使用PowerShell命令终止所有quick_launcher进程
            subprocess.run([
                'powershell.exe', 
                '-Command', 
                'Get-Process quick_launcher -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue'
            ], check=False, shell=True)
            print("已终止所有正在运行的quick_launcher进程")
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
    # 添加Windows特定库
    env.Append(LIBS=['shell32', 'user32', 'gdi32', 'comctl32', 'imm32'])
    
    # 检查SFML是否可用
    sfml_available = False
    sfml_include_paths = [
        'C:\\SFML-2.5.1\\include',
        'D:\\SFML-2.5.1\\include',
        'C:\\Program Files\\SFML\\include',
        'D:\\Program Files\\SFML\\include'
    ]
    
    sfml_lib_paths = [
        'C:\\SFML-2.5.1\\lib',
        'D:\\SFML-2.5.1\\lib',
        'C:\\Program Files\\SFML\\lib',
        'D:\\Program Files\\SFML\\lib'
    ]
    
    # 尝试添加SFML路径
    for include_path in sfml_include_paths:
        if os.path.exists(include_path):
            env.Append(CPPPATH=[include_path])
            print(f"添加SFML头文件路径: {include_path}")
            sfml_available = True
            break
    
    for lib_path in sfml_lib_paths:
        if os.path.exists(lib_path):
            env.Append(LIBPATH=[lib_path])
            print(f"添加SFML库路径: {lib_path}")
            sfml_available = True
            break
    
    if sfml_available:
        # 添加SFML库
        env.Append(LIBS=['sfml-graphics', 'sfml-window', 'sfml-system'])
        print("SFML库已配置，将编译SFML版本")
    else:
        print("警告：未找到SFML库，将编译Windows API版本")
        # 使用Windows API版本
        sources = ['gui_main.cpp', 'command_handler.cpp']
    
    # 尝试检测可用的编译器
    try:
        # 尝试使用Visual Studio编译器
        env.Tool('msvc')
        env['CXXFLAGS'] = ['/EHsc', '/W3']
        print("使用Visual Studio编译器")
    except:
        # 如果Visual Studio不可用，尝试使用其他编译器设置
        print("未检测到Visual Studio编译器，使用默认编译设置")
        env['CXXFLAGS'] = []
else:
    # 非Windows环境
    env['CXXFLAGS'] = ['-std=c++17', '-Wall', '-Wextra']
    # 非Windows环境下的SFML库
    env.Append(LIBS=['sfml-graphics', 'sfml-window', 'sfml-system'])

# 源文件 - 默认使用SFML版本，如果SFML不可用则使用Windows API版本
if 'sfml_available' in locals() and sfml_available:
    sources = ['sfml_main.cpp']
else:
    sources = ['gui_main.cpp', 'command_handler.cpp']

# Windows GUI应用程序设置
if os.name == 'nt':
    # 设置为GUI子系统
    env.Append(LINKFLAGS=['/SUBSYSTEM:WINDOWS', '/ENTRY:WinMainCRTStartup'])

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
target_path = os.path.join(bin_dir, 'quick_launcher')

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

# 使用对象文件构建可执行文件
executable = env.Program(target=target_path, source=object_files)

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

# 设置清理目标
Clean(executable, os.path.join(bin_dir, 'quick_launcher.exe'))
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
import os
import subprocess

# 命令行参数解析
AddOption('--language', dest='language', type='string', default='chinese',
          help='编译语言版本: chinese 或 english')
          
AddOption('--build-all', dest='build_all', action='store_true',
          help='同时编译中英文两个版本')

# 获取语言配置
def get_language_config():
    lang = GetOption('language')
    if lang not in ['chinese', 'english']:
        print("错误：语言参数必须是 'chinese' 或 'english'")
        Exit(1)
    
    defines = []
    defines_mapping = {
        'chinese': ['LANGUAGE_CHINESE=1'],
        'english': ['LANGUAGE_ENGLISH=1']
    }
    
    return lang, defines_mapping.get(lang, ['LANGUAGE_CHINESE=1'])

# 获取输出文件名
def get_output_filename(lang):
    return f'funny_quick_{lang}'

# 创建基本环境，确保包含所有必要的工具
env = Environment(tools=['default'])

# 获取语言设置
lang, defines = get_language_config()
print(f"编译语言版本: {lang}")

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
def ensure_obj_directory(lang=None):
    obj_dir = 'obj'
    if lang:
        obj_dir = f'obj_{lang}'
    if not os.path.exists(obj_dir):
        os.makedirs(obj_dir)
        print(f"已创建obj目录: {os.path.abspath(obj_dir)}")
    return obj_dir

# Windows环境特殊设置
if os.name == 'nt':
    # 添加Windows特定库
    env.Append(LIBS=['shell32', 'user32', 'gdi32', 'comctl32', 'imm32'])
    
    # 添加资源编译器支持
    env['RC'] = 'rc.exe'
    env['RCCOM'] = '$RC $RCFLAGS /fo$TARGET $SOURCES'
    env['RCFLAGS'] = '/c65001'  # 设置UTF-8编码
    env['BUILDERS']['RES'] = Builder(action='$RCCOM', suffix='.res', src_suffix='.rc')
    
    # 检查SFML是否可用
    sfml_available = False
    sfml_include_paths = [
        'C:\\SFML-2.5.1\\include',
        'D:\\SFML-2.5.1\\include',
        'E:\\BaiduSyncdisk\\CPP\\common\\SFML-2.5.1\\include',
        'C:\\SFML-2.6.0\\include',
        'D:\\SFML-2.6.0\\include',
        'C:\\Program Files\\SFML\\include',
        'D:\\Program Files\\SFML\\include',
        'C:\\SFML\\include',
        'D:\\SFML\\include'
    ]
    
    sfml_lib_paths = [
        'C:\\SFML-2.5.1\\lib',
        'D:\\SFML-2.5.1\\lib',
        'E:\\BaiduSyncdisk\\CPP\\common\\SFML-2.5.1\\lib',
        'C:\\SFML-2.6.0\\lib',
        'D:\\SFML-2.6.0\\lib',
        'C:\\Program Files\\SFML\\lib',
        'D:\\Program Files\\SFML\\lib',
        'C:\\SFML\\lib',
        'D:\\SFML\\lib'
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
        sources = ['gui_main.cpp', 'command_handler.cpp', 'translation_manager.cpp']
    
    # 尝试检测可用的编译器
    try:
        # 尝试使用Visual Studio编译器
        env.Tool('msvc')
        env['CXXFLAGS'] = ['/EHsc', '/W3', '/utf-8', '/std:c++17']  # /utf-8选项已包含完整的UTF-8支持
        print("使用Visual Studio编译器，启用UTF-8支持")
    except:
        # 如果Visual Studio不可用，尝试使用其他编译器设置
        print("未检测到Visual Studio编译器，使用默认编译设置")
        env['CXXFLAGS'] = ['/utf-8']  # 使用/utf-8选项
    
else:
    # 非Windows环境
    env['CXXFLAGS'] = ['-std=c++17', '-Wall', '-Wextra']
    # 非Windows环境下的SFML库
    env.Append(LIBS=['sfml-graphics', 'sfml-window', 'sfml-system'])

# 添加UTF-8编译选项
env.Append(CCFLAGS=['/utf-8'])
# 添加语言相关的预处理器定义
env.Append(CPPDEFINES=defines)

# 检查编译目标
target_type = ARGUMENTS.get('target', 'default')

if target_type == 'test_calc':
    sources = ['test_calc_simple.cpp']
    resource_files = []
elif target_type == 'sfml':
    sources = ['sfml_main.cpp']
    resource_files = []
elif 'sfml_available' in locals() and sfml_available:
    sources = ['sfml_main.cpp']
    resource_files = []
else:
    sources = ['src/gui_main.cpp', 'src/command_handler.cpp', 'src/translation_manager.cpp']
    
    # Windows环境下添加资源文件
    if os.name == 'nt':
        resource_files = ['src/resource.rc']
    else:
        resource_files = ['src/resource.rc']

# Windows应用程序设置
if os.name == 'nt':
    # 根据是否使用SFML设置不同的子系统
    if 'sfml_available' in locals() and sfml_available:
        # SFML版本使用控制台子系统
        env.Append(LINKFLAGS=['/SUBSYSTEM:CONSOLE'])
    else:
        # Windows API版本使用GUI子系统
        env.Append(LINKFLAGS=['/SUBSYSTEM:WINDOWS', '/ENTRY:WinMainCRTStartup'])

# 检查是否需要编译所有版本
build_all = GetOption('build_all')

if build_all:
    print("开始同时编译中英文两个版本...")
    build_targets = []
    
    # 编译中文版
    print("=== 编译中文版 ===")
    env_chinese = env.Clone()
    env_chinese['CPPDEFINES'] = ['LANGUAGE_CHINESE=1']
    obj_dir_chinese = ensure_obj_directory('chinese')
    env_chinese['OBJDIR'] = obj_dir_chinese
    target_path_chinese = os.path.join(ensure_bin_directory(), get_output_filename('chinese'))
    
    # 为中文版创建对象文件
    object_files_chinese = []
    for src in sources:
        base_name = os.path.splitext(os.path.basename(src))[0]
        obj_name = os.path.join(obj_dir_chinese, base_name + '.obj')
        obj = env_chinese.Object(target=obj_name, source=src)
        object_files_chinese.append(obj)
    
    # 为中文版创建资源文件
    resource_objects_chinese = []
    for rc in resource_files:
        base_name = os.path.splitext(os.path.basename(rc))[0]
        res_name = os.path.join(obj_dir_chinese, base_name + '.res')
        res = env_chinese.RES(target=res_name, source=rc)
        resource_objects_chinese.append(res)
    
    # 构建中文版可执行文件
    all_objects_chinese = object_files_chinese + resource_objects_chinese
    executable_chinese = env_chinese.Program(target=target_path_chinese, source=all_objects_chinese)
    build_targets.append(executable_chinese)
    
    # 复制中文版图标文件
    if os.path.exists('app_icon.ico'):
        icon_target_chinese = os.path.join(ensure_bin_directory(), 'app_icon_chinese.ico')
        icon_copy_chinese = env_chinese.Command(icon_target_chinese, 'app_icon.ico', Copy('$TARGET', '$SOURCE'))
        env_chinese.Depends(executable_chinese, icon_copy_chinese)
    
    # 编译英文版
    print("=== 编译英文版 ===")
    env_english = env.Clone()
    env_english['CPPDEFINES'] = ['LANGUAGE_ENGLISH=1']
    obj_dir_english = ensure_obj_directory('english')
    env_english['OBJDIR'] = obj_dir_english
    target_path_english = os.path.join(ensure_bin_directory(), get_output_filename('english'))
    
    # 为英文版创建对象文件
    object_files_english = []
    for src in sources:
        base_name = os.path.splitext(os.path.basename(src))[0]
        obj_name = os.path.join(obj_dir_english, base_name + '.obj')
        obj = env_english.Object(target=obj_name, source=src)
        object_files_english.append(obj)
    
    # 为英文版创建资源文件
    resource_objects_english = []
    for rc in resource_files:
        base_name = os.path.splitext(os.path.basename(rc))[0]
        res_name = os.path.join(obj_dir_english, base_name + '.res')
        res = env_english.RES(target=res_name, source=rc)
        resource_objects_english.append(res)
    
    # 构建英文版可执行文件
    all_objects_english = object_files_english + resource_objects_english
    executable_english = env_english.Program(target=target_path_english, source=all_objects_english)
    build_targets.append(executable_english)
    
    # 复制英文版图标文件
    if os.path.exists('app_icon.ico'):
        icon_target_english = os.path.join(ensure_bin_directory(), 'app_icon_english.ico')
        icon_copy_english = env_english.Command(icon_target_english, 'app_icon.ico', Copy('$TARGET', '$SOURCE'))
        env_english.Depends(executable_english, icon_copy_english)
    
    # 显示编译结果
    print("=== 编译完成 ===")
    print(f"中文版: {target_path_chinese}")
    print(f"英文版: {target_path_english}")
    
    # 设置默认目标
    Default(build_targets)
else:
    # 编译单个版本（默认行为）
    print("=== 编译单个版本 ===")
    bin_dir = ensure_bin_directory()
    obj_dir = ensure_obj_directory()
    kill_running_processes()
    
    # 设置对象文件输出目录
    env['OBJDIR'] = obj_dir
    # 为每个源文件设置输出路径
    env['OBJSUFFIX'] = '.obj'
    env['PROGSUFFIX'] = '.exe'
    
    # 构建可执行文件到bin目录
    target_path = os.path.join(bin_dir, get_output_filename(lang))
    
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
        # 构建资源文件
        res = env.RES(target=res_name, source=rc)
        resource_objects.append(res)
    
    # 使用对象文件和资源文件构建可执行文件
    all_objects = object_files + resource_objects
    executable = env.Program(target=target_path, source=all_objects)
    
    # 复制图标文件到bin目录
    if os.path.exists('app_icon.ico'):
        icon_target = os.path.join(bin_dir, f'app_icon_{lang}.ico')
        # 使用env.Command创建复制任务
        icon_copy = env.Command(icon_target, 'app_icon.ico', Copy('$TARGET', '$SOURCE'))
        # 确保图标复制在构建时执行
        env.Depends(executable, icon_copy)
        print(f"将复制图标文件到bin目录: {icon_target}")
    else:
        print("警告：未找到app_icon.ico文件")
    
    # 如果使用SFML，复制所需的DLL文件到bin目录
    if 'sfml_available' in locals() and sfml_available:
        sfml_bin_paths = [
            'E:\\BaiduSyncdisk\\CPP\\common\\SFML-2.5.1\\bin',
            'C:\\SFML\\bin',
            'D:\\SFML\\bin'
        ]
        
        dll_files_to_copy = ['sfml-graphics-2.dll', 'sfml-window-2.dll', 'sfml-system-2.dll']
        
        for dll_file in dll_files_to_copy:
            dll_copied = False
            for sfml_bin_path in sfml_bin_paths:
                source_dll = os.path.join(sfml_bin_path, dll_file)
                if os.path.exists(source_dll):
                    target_dll = os.path.join(bin_dir, dll_file)
                    dll_copy = env.Command(target_dll, source_dll, Copy('$TARGET', '$SOURCE'))
                    env.Depends(executable, dll_copy)
                    print(f"将复制DLL文件: {dll_file} 从 {sfml_bin_path} 到 {bin_dir}")
                    dll_copied = True
                    break
            
            if not dll_copied:
                print(f"警告：未找到DLL文件 {dll_file}")
    
    # 设置清理目标
    Clean(executable, os.path.join(bin_dir, get_output_filename(lang)))
    # 清理obj目录中的所有对象文件
    for src in sources:
        base_name = os.path.splitext(os.path.basename(src))[0]
        obj_name = os.path.join(obj_dir, base_name + '.obj')
        Clean(executable, obj_name)
    # 清理复制的图标文件
    if os.path.exists('app_icon.ico'):
        Clean(executable, os.path.join(bin_dir, f'app_icon_{lang}.ico'))

print("=== 编译说明 ===")
print("使用 'scons' 构建单个版本（默认中文版）")
print("使用 'scons --language=english' 构建英文版")
print("使用 'scons --build-all' 同时构建中英文两个版本")
print("使用 'scons -c' 清理项目")
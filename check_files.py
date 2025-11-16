import os

print('编译前检查：')
print('1. 检查internationalization.h是否存在...')
if os.path.exists('internationalization.h'):
    print('   ✓ internationalization.h存在')
    # 检查是否包含必要的中英文定义
    with open('internationalization.h', 'r', encoding='utf-8') as f:
        content = f.read()
        if 'LANGUAGE_CHINESE' in content:
            print('   ✓ 包含中文语言定义')
        if 'LANGUAGE_ENGLISH' in content:
            print('   ✓ 包含英文语言定义')
else:
    print('   ✗ internationalization.h不存在')

print('2. 检查gui_main.cpp是否正确包含国际化头文件...')
with open('gui_main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()
    if '#include "internationalization.h"' in content:
        print('   ✓ 正确包含internationalization.h')
    else:
        print('   ✗ 未包含internationalization.h')

print('3. 检查build_dual_language.bat是否存在...')
if os.path.exists('build_dual_language.bat'):
    print('   ✓ 编译脚本存在')
else:
    print('   ✗ 编译脚本不存在')

print('检查完成！')

#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>

// 定义外部函数
extern void ShowBookmarkDialog();
extern void EnterBookmarkMode();
extern void ExitBookmarkMode();

int main() {
    printf("测试网址收藏模式中的中文显示\n");
    printf("1. 调用EnterBookmarkMode进入网址收藏模式\n");
    
    // 直接调用EnterBookmarkMode
    EnterBookmarkMode();
    
    printf("2. 等待5秒，检查中文显示\n");
    Sleep(5000);
    
    printf("3. 调用ExitBookmarkMode退出网址收藏模式\n");
    ExitBookmarkMode();
    
    printf("测试完成\n");
    return 0;
}

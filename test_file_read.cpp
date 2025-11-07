#include <windows.h>
#include <stdio.h>
#include <string>

int main() {
    // 设置控制台输出编码为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    printf("测试文件读取...\n");
    
    // 打开历史文件
    FILE* file = fopen("data\\calculation_history.txt", "r, ccs=UTF-8");
    if (!file) {
        printf("无法打开历史文件进行读取\n");
        return 1;
    }
    
    printf("成功打开历史文件\n");
    
    // 检查文件是否为空
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("文件大小为 %ld 字节\n", fileSize);
    
    if (fileSize == 0) {
        printf("文件为空\n");
        fclose(file);
        return 0;
    }
    
    // 读取历史记录
    char buffer[1024];
    int lineCount = 0;
    bool isFirstLine = true;
    
    while (fgets(buffer, sizeof(buffer), file)) {
        lineCount++;
        
        // 移除换行符
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        
        // 处理第一行的BOM标记
        if (isFirstLine && len >= 3 && (unsigned char)buffer[0] == 0xEF && 
            (unsigned char)buffer[1] == 0xBB && (unsigned char)buffer[2] == 0xBF) {
            printf("检测到UTF-8 BOM标记，跳过\n");
            memmove(buffer, buffer + 3, len - 2);
            buffer[len - 2] = '\0';
            len -= 3;
        }
        isFirstLine = false;
        
        // 跳过空行
        if (len == 0) {
            printf("跳过空行\n");
            continue;
        }
        
        printf("读取第%d行: '%s'\n", lineCount, buffer);
    }
    
    fclose(file);
    printf("文件读取完成，共读取 %d 行\n", lineCount);
    
    return 0;
}
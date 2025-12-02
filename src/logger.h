#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

// 日志功能模块
void LogToFile(const char* message);
void CloseLogFile();

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H



import ctypes
import time

# 加载程序
dll = ctypes.windll.LoadLibrary("E:\\github\\funny_quick\\bin\\quick_launcher.exe")

# 这里我们无法直接调用函数，因为这是一个EXE文件
# 所以我们需要手动检查日志文件
print("无法直接调用EnterBookmarkMode函数，因为这是一个EXE文件")
print("请手动打开网址管理对话框，然后点击关闭按钮进入网址收藏模式")
print("然后检查列表框中的中文显示是否正常")

import win32com.client

speaker = win32com.client.Dispatch("SAPI.SpVoice")
speaker.Speak("任务运行完毕，过来看看！")
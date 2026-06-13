#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""FunnyQuick 的 tscn 可视化编辑启动器。"""

from __future__ import annotations

import subprocess
import sys
import threading
from datetime import datetime
from pathlib import Path
import tkinter as tk
from tkinter import messagebox, scrolledtext


ROOT_DIR = Path(__file__).resolve().parent
EDITOR_EXE = ROOT_DIR.parent / "godot_ui" / "godot-ui-standalone-direct2d" / "build" / "Debug" / "ui_editor.exe"
FUNNY_QUICK_EXE = ROOT_DIR / "bin" / "funny_quick.exe"
GODOT_UI_ROOT = ROOT_DIR.parent / "godot_ui" / "godot-ui-standalone-direct2d"
SCENE_FILES = {
    "主界面": ROOT_DIR / "ui" / "funny_quick_main.tscn",
    "设置弹窗": ROOT_DIR / "ui" / "components" / "settings_popup.tscn",
    "快捷方式弹窗": ROOT_DIR / "ui" / "components" / "shortcut_edit_popup.tscn",
}


def normalize_path(path: Path) -> str:
    """把路径统一成 Windows 风格字符串，便于展示和传参。"""
    return str(path.resolve())


def append_log(log_widget: scrolledtext.ScrolledText, message: str) -> None:
    """向日志区追加一条带时间戳的文本。"""
    timestamp = datetime.now().strftime("%H:%M:%S")
    log_widget.insert("end", f"[{timestamp}] {message}\n")
    log_widget.see("end")


def set_status(status_var: tk.StringVar, text: str, log_widget: scrolledtext.ScrolledText | None = None) -> None:
    """统一更新状态栏，并按需同步写入日志。"""
    status_var.set(f"状态：{text}")
    if log_widget is not None:
        append_log(log_widget, text)


def launch_editor_for_scene(scene_path: Path, status_var: tk.StringVar, log_widget: scrolledtext.ScrolledText) -> None:
    """启动可视化编辑器并打开指定的 tscn 场景。"""
    if not EDITOR_EXE.exists():
        messagebox.showerror(
            "编辑器不存在",
            "未找到可视化编辑器，请先编译 ui_editor。\n\n"
            f"{normalize_path(EDITOR_EXE)}",
        )
        set_status(status_var, "编辑器不存在，请先编译 ui_editor。", log_widget)
        return

    if not scene_path.exists():
        messagebox.showerror(
            "场景不存在",
            "未找到目标 tscn 文件：\n\n"
            f"{normalize_path(scene_path)}",
        )
        set_status(status_var, f"场景不存在 -> {scene_path.name}", log_widget)
        return

    try:
        subprocess.Popen(
            [normalize_path(EDITOR_EXE), "--scene", normalize_path(scene_path)],
            cwd=str(EDITOR_EXE.parent),
        )
        set_status(status_var, f"已打开 {scene_path.name}", log_widget)
    except Exception as exc:  # noqa: BLE001
        messagebox.showerror("启动失败", f"启动编辑器失败：\n\n{exc}")
        set_status(status_var, f"启动失败 -> {scene_path.name}", log_widget)


def launch_all_scenes(status_var: tk.StringVar, log_widget: scrolledtext.ScrolledText) -> None:
    """依次打开全部 3 个 tscn 场景到可视化编辑器。"""
    for scene_path in SCENE_FILES.values():
        launch_editor_for_scene(scene_path, status_var, log_widget)
    set_status(status_var, "已尝试打开全部场景。", log_widget)


def launch_funny_quick(status_var: tk.StringVar, log_widget: scrolledtext.ScrolledText) -> None:
    """启动 FunnyQuick 主程序，方便边改边看效果。"""
    if not FUNNY_QUICK_EXE.exists():
        messagebox.showerror(
            "程序不存在",
            "未找到 FunnyQuick 主程序，请先编译项目。\n\n"
            f"{normalize_path(FUNNY_QUICK_EXE)}",
        )
        set_status(status_var, "FunnyQuick 主程序不存在。", log_widget)
        return

    try:
        subprocess.Popen([normalize_path(FUNNY_QUICK_EXE)], cwd=str(FUNNY_QUICK_EXE.parent))
        set_status(status_var, "已启动 FunnyQuick 主程序。", log_widget)
    except Exception as exc:  # noqa: BLE001
        messagebox.showerror("启动失败", f"启动 FunnyQuick 失败：\n\n{exc}")
        set_status(status_var, "启动 FunnyQuick 失败。", log_widget)


def run_command_async(
    command: list[str],
    cwd: Path,
    title: str,
    status_var: tk.StringVar,
    log_widget: scrolledtext.ScrolledText,
) -> None:
    """异步执行命令并把输出持续写到日志区域。"""

    def worker() -> None:
        root = log_widget.winfo_toplevel()
        root.after(0, lambda: set_status(status_var, f"{title}开始...", log_widget))
        root.after(0, lambda: append_log(log_widget, f"命令: {' '.join(command)}"))
        try:
            process = subprocess.Popen(
                command,
                cwd=str(cwd),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                bufsize=1,
            )
            assert process.stdout is not None
            for line in process.stdout:
                root.after(0, lambda text=line.rstrip(): append_log(log_widget, text))
            exit_code = process.wait()
            if exit_code == 0:
                root.after(0, lambda: set_status(status_var, f"{title}完成。", log_widget))
            else:
                root.after(0, lambda: set_status(status_var, f"{title}失败，退出码 {exit_code}。", log_widget))
        except Exception as exc:  # noqa: BLE001
            root.after(0, lambda: set_status(status_var, f"{title}异常：{exc}", log_widget))

    threading.Thread(target=worker, daemon=True).start()


def build_funny_quick(status_var: tk.StringVar, log_widget: scrolledtext.ScrolledText) -> None:
    """编译 FunnyQuick 项目，方便修改场景后快速回归。"""
    run_command_async(["scons"], ROOT_DIR, "编译 FunnyQuick", status_var, log_widget)


def build_ui_editor(status_var: tk.StringVar, log_widget: scrolledtext.ScrolledText) -> None:
    """编译 ui_editor，确保可视化编辑器可用。"""
    run_command_async(["scons", "-j1", "config=debug", "ui_editor"], GODOT_UI_ROOT, "编译可视化编辑器", status_var, log_widget)


def open_scene_folder(status_var: tk.StringVar, log_widget: scrolledtext.ScrolledText) -> None:
    """打开 tscn 场景所在目录，方便手动查看和管理文件。"""
    scene_dir = ROOT_DIR / "ui"
    try:
        subprocess.Popen(["explorer", normalize_path(scene_dir)])
        set_status(status_var, "已打开 ui 场景目录。", log_widget)
    except Exception as exc:  # noqa: BLE001
        messagebox.showerror("打开目录失败", f"无法打开目录：\n\n{exc}")
        set_status(status_var, "打开 ui 目录失败。", log_widget)


def build_main_window() -> tk.Tk:
    """创建启动器主窗口，并绑定 3 个 tscn 的可视化编辑按钮。"""
    root = tk.Tk()
    root.title("FunnyQuick UI 场景编辑器")
    root.geometry("780x560")
    root.minsize(700, 500)

    status_var = tk.StringVar(value="状态：就绪。")

    title = tk.Label(
        root,
        text="FunnyQuick 可视化场景编辑",
        font=("Microsoft YaHei UI", 16, "bold"),
        anchor="w",
    )
    title.pack(fill="x", padx=18, pady=(16, 4))

    subtitle = tk.Label(
        root,
        text="点击下面的按钮，可直接用 ui_editor 打开 3 个 tscn 场景。",
        anchor="w",
        justify="left",
    )
    subtitle.pack(fill="x", padx=18, pady=(0, 10))

    top_actions = tk.Frame(root)
    top_actions.pack(fill="x", padx=18, pady=(0, 10))

    tk.Button(
        top_actions,
        text="打开全部场景",
        width=14,
        command=lambda: launch_all_scenes(status_var, log_widget),
    ).pack(side="left")

    tk.Button(
        top_actions,
        text="启动 FunnyQuick",
        width=14,
        command=lambda: launch_funny_quick(status_var, log_widget),
    ).pack(side="left", padx=(8, 0))

    tk.Button(
        top_actions,
        text="编译 FunnyQuick",
        width=14,
        command=lambda: build_funny_quick(status_var, log_widget),
    ).pack(side="left", padx=(8, 0))

    tk.Button(
        top_actions,
        text="编译编辑器",
        width=14,
        command=lambda: build_ui_editor(status_var, log_widget),
    ).pack(side="left", padx=(8, 0))

    card = tk.Frame(root, bd=1, relief="groove", padx=14, pady=14)
    card.pack(fill="x", padx=18, pady=(0, 12))

    for scene_name, scene_path in SCENE_FILES.items():
        row = tk.Frame(card)
        row.pack(fill="x", pady=6)

        info = tk.Label(
            row,
            text=f"{scene_name}\n{normalize_path(scene_path)}",
            justify="left",
            anchor="w",
        )
        info.pack(side="left", fill="x", expand=True)

        button = tk.Button(
            row,
            text="可视化编辑",
            width=12,
            command=lambda p=scene_path: launch_editor_for_scene(p, status_var, log_widget),
        )
        button.pack(side="right", padx=(12, 0))

    log_card = tk.Frame(root, bd=1, relief="groove", padx=8, pady=8)
    log_card.pack(fill="both", expand=True, padx=18, pady=(0, 12))

    log_title = tk.Label(log_card, text="操作日志", anchor="w")
    log_title.pack(fill="x", pady=(0, 6))

    log_widget = scrolledtext.ScrolledText(log_card, height=12, wrap="word")
    log_widget.pack(fill="both", expand=True)
    append_log(log_widget, "启动器已就绪。可直接点击按钮打开场景或编译项目。")

    bottom = tk.Frame(root)
    bottom.pack(fill="x", padx=18, pady=(0, 16))

    folder_button = tk.Button(
        bottom,
        text="打开场景目录",
        width=12,
        command=lambda: open_scene_folder(status_var, log_widget),
    )
    folder_button.pack(side="left")

    status = tk.Label(
        bottom,
        textvariable=status_var,
        anchor="w",
        justify="left",
    )
    status.pack(side="left", fill="x", expand=True, padx=(12, 0))

    return root


def main() -> int:
    """程序入口，启动 FunnyQuick 的 Python 场景编辑器界面。"""
    if sys.platform != "win32":
        print("该工具当前仅针对 Windows 环境。")
        return 1

    app = build_main_window()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

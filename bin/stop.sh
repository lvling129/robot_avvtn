#!/bin/bash

# 替换为你的C++可执行文件名称
PROGRAM_NAME="robot_avvtn"
PID_FILE="${PROGRAM_NAME}.pid"

# 检查PID文件是否存在
if [ ! -f "$PID_FILE" ]; then
    echo "错误：未找到PID文件，程序 ${PROGRAM_NAME} 可能未运行！"
    exit 1
fi

PID=$(cat "$PID_FILE")
# 检查进程是否存在
if ! ps -p "$PID" > /dev/null 2>&1; then
    echo "错误：PID $PID 对应的进程不存在，清理无效PID文件..."
    rm -f "$PID_FILE"
    exit 1
fi

# 停止进程（先尝试优雅终止，失败则强制杀死）
echo "正在停止 ${PROGRAM_NAME}（PID: $PID）..."
kill "$PID" > /dev/null 2>&1
sleep 2

# 验证是否停止成功
if ps -p "$PID" > /dev/null 2>&1; then
    echo "优雅停止失败，强制杀死进程..."
    kill -9 "$PID" > /dev/null 2>&1
fi

# 清理PID文件
rm -f "$PID_FILE"
echo "程序 ${PROGRAM_NAME} 已停止！"
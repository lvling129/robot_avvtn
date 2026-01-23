#!/bin/bash

# 替换为你的C++可执行文件名称
PROGRAM_NAME="robot_avvtn"
PID_FILE="${PROGRAM_NAME}.pid"

# 定义颜色输出函数
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印带颜色的信息
echo_color() {
    echo -e "${1}${2}${NC}"
}

# 检查PID文件是否存在
if [ ! -f "$PID_FILE" ]; then
    echo_color $YELLOW "未找到PID文件，尝试查找 ${PROGRAM_NAME} 进程..."
    
    # 尝试通过进程名查找
    PIDS=$(pgrep -f "./${PROGRAM_NAME}")
    if [ -z "$PIDS" ]; then
        echo_color $RED "错误：程序 ${PROGRAM_NAME} 未运行！"
        exit 1
    else
        echo_color $YELLOW "发现 ${PROGRAM_NAME} 进程: $PIDS"
        # 使用第一个找到的PID
        MAIN_PID=$(echo $PIDS | awk '{print $1}')
        echo_color $BLUE "使用主进程 PID: $MAIN_PID"
        PID=$MAIN_PID
    fi
else
    PID=$(cat "$PID_FILE")
fi

# 检查进程是否存在
if ! ps -p "$PID" > /dev/null 2>&1; then
    echo_color $YELLOW "PID $PID 对应的进程不存在，清理残留文件..."
    rm -f "$PID_FILE"
    
    # 尝试杀死所有同名进程
    PIDS=$(pgrep -f "./${PROGRAM_NAME}" 2>/dev/null)
    if [ -n "$PIDS" ]; then
        echo_color $YELLOW "发现残留进程，正在清理..."
        pkill -f "./${PROGRAM_NAME}"
    fi
    exit 0
fi

# 获取进程信息
echo_color $BLUE "正在停止 ${PROGRAM_NAME}（PID: $PID）..."
ps -fp "$PID" 2>/dev/null | grep -v "UID" || echo_color $YELLOW "无法获取进程详细信息"

# 停止进程（先尝试优雅终止，失败则强制杀死）
echo_color $BLUE "发送终止信号..."
kill "$PID" > /dev/null 2>&1

# 等待进程退出（最长10秒）
TIMEOUT=10
COUNTER=0
while ps -p "$PID" > /dev/null 2>&1 && [ $COUNTER -lt $TIMEOUT ]; do
    echo_color $YELLOW "等待进程退出... ($((TIMEOUT-COUNTER))秒后强制终止)"
    sleep 1
    COUNTER=$((COUNTER + 1))
done

# 验证是否停止成功
if ps -p "$PID" > /dev/null 2>&1; then
    echo_color $RED "优雅停止失败，强制杀死进程..."
    kill -9 "$PID" > /dev/null 2>&1
    sleep 1
fi

# 清理PID文件
if [ -f "$PID_FILE" ]; then
    rm -f "$PID_FILE"
fi

# 检查是否还有残留进程
PIDS=$(pgrep -f "./${PROGRAM_NAME}" 2>/dev/null)
if [ -n "$PIDS" ]; then
    echo_color $YELLOW "发现残留进程 ($PIDS)，正在清理..."
    pkill -f "./${PROGRAM_NAME}"
    sleep 1
fi

echo_color $GREEN "程序 ${PROGRAM_NAME} 已成功停止！"

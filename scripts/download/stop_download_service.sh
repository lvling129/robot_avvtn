#!/bin/bash

# 下载服务停止脚本
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROGRAM_NAME="download_service.py"
PID_FILE="${SCRIPT_DIR}/download_service.pid"

# 定义颜色输出函数
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的信息
echo_color() {
    echo -e "${1}${2}${NC}"
}

echo_color $BLUE "========================================"
echo_color $BLUE "   ROS2 下载服务停止脚本"
echo_color $BLUE "========================================"

# 检查PID文件是否存在
if [ ! -f "$PID_FILE" ]; then
    echo_color $YELLOW "未找到PID文件，尝试查找进程..."
    
    # 匹配本目录下的 download_service.py
    PIDS=$(pgrep -f "download/download_service.py")
    if [ -z "$PIDS" ]; then
        echo_color $GREEN "下载服务未运行"
        exit 0
    else
        echo_color $YELLOW "发现进程: $PIDS"
        PID=$(echo $PIDS | awk '{print $1}')
    fi
else
    PID=$(cat "$PID_FILE")
fi

# 检查进程是否存在
if ! ps -p "$PID" > /dev/null 2>&1; then
    echo_color $YELLOW "PID $PID 对应的进程不存在，清理残留文件..."
    rm -f "$PID_FILE"
    
    # 尝试杀死本目录下的下载服务进程
    PIDS=$(pgrep -f "download/download_service.py" 2>/dev/null)
    if [ -n "$PIDS" ]; then
        echo_color $YELLOW "发现残留进程，正在清理..."
        pkill -f "download/download_service.py"
    fi
    echo_color $GREEN "清理完成"
    exit 0
fi

# 获取进程信息
echo_color $BLUE "正在停止下载服务（PID: $PID）..."
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
PIDS=$(pgrep -f "download/download_service.py" 2>/dev/null)
if [ -n "$PIDS" ]; then
    echo_color $YELLOW "发现残留进程 ($PIDS)，正在清理..."
    pkill -f "download/download_service.py"
    sleep 1
fi

echo_color $GREEN "下载服务已成功停止！"

#!/bin/bash

# 下载服务启动脚本
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROGRAM_NAME="download_service.py"
PID_FILE="${SCRIPT_DIR}/download_service.pid"
LOG_FILE="${SCRIPT_DIR}/download_service.log"

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

# 检查Python环境
check_python() {
    if command -v python3 &> /dev/null; then
        PYTHON_CMD="python3"
    elif command -v python &> /dev/null; then
        PYTHON_CMD="python"
    else
        echo_color $RED "错误：未找到Python解释器"
        exit 1
    fi
    echo_color $BLUE "使用Python: $($PYTHON_CMD --version)"
}

# 检查依赖
check_dependencies() {
    echo_color $YELLOW "检查依赖..."
    
    # 检查rclpy
    if ! $PYTHON_CMD -c "import rclpy" 2>/dev/null; then
        echo_color $RED "错误：未安装rclpy，请确保ROS2环境已正确配置"
        echo_color $YELLOW "提示：请运行 'source /opt/ros/<ros2_distro>/setup.bash'"
        exit 1
    fi
    
    # 检查aiohttp
    if ! $PYTHON_CMD -c "import aiohttp" 2>/dev/null; then
        echo_color $YELLOW "正在安装aiohttp..."
        $PYTHON_CMD -m pip install aiohttp -q
        if [ $? -ne 0 ]; then
            echo_color $RED "错误：无法安装aiohttp"
            exit 1
        fi
    fi
    
    echo_color $GREEN "依赖检查通过"
}

# 检查并停止正在运行的程序
stop_existing() {
    echo_color $YELLOW "检查现有进程..."
    
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if ps -p "$PID" > /dev/null 2>&1; then
            echo_color $BLUE "发现程序正在运行 (PID: $PID)，尝试停止..."
            kill "$PID" > /dev/null 2>&1
            
            # 等待最多5秒让程序优雅退出
            COUNTER=0
            while ps -p "$PID" > /dev/null 2>&1 && [ $COUNTER -lt 10 ]; do
                sleep 0.5
                COUNTER=$((COUNTER + 1))
            done
            
            # 如果进程还在，强制杀死
            if ps -p "$PID" > /dev/null 2>&1; then
                echo_color $YELLOW "强制杀死进程..."
                kill -9 "$PID" > /dev/null 2>&1
            fi
            
            rm -f "$PID_FILE"
            echo_color $GREEN "已停止旧进程"
        else
            rm -f "$PID_FILE"
            echo_color $YELLOW "清理无效的PID文件"
        fi
    fi
    
    # 检查残留进程（匹配本目录下的 download_service.py）
    PIDS=$(pgrep -f "download/download_service.py" 2>/dev/null)
    if [ -n "$PIDS" ]; then
        echo_color $YELLOW "发现残留进程，正在清理..."
        pkill -f "download/download_service.py"
        sleep 1
    fi
}

# 主流程
echo_color $GREEN "========================================"
echo_color $GREEN "   ROS2 下载服务启动脚本"
echo_color $GREEN "========================================"

check_python
check_dependencies
stop_existing

# 等待确保进程完全释放资源
sleep 1

# 备份旧日志
if [ -f "$LOG_FILE" ]; then
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    BACKUP_LOG="${LOG_FILE}.backup_${TIMESTAMP}"
    echo_color $BLUE "备份旧日志: $LOG_FILE -> $BACKUP_LOG"
    cp "$LOG_FILE" "$BACKUP_LOG"
    > "$LOG_FILE"
else
    echo_color $BLUE "创建新的日志文件: $LOG_FILE"
    touch "$LOG_FILE"
fi

# 启动服务
echo_color $YELLOW "正在启动下载服务..."
cd "$SCRIPT_DIR"
$PYTHON_CMD "$PROGRAM_NAME" >> "$LOG_FILE" 2>&1 &
NEW_PID=$!
echo $NEW_PID > "$PID_FILE"

# 验证启动是否成功
sleep 2
if ps -p $NEW_PID > /dev/null 2>&1; then
    echo_color $GREEN "下载服务启动成功！"
    echo_color $BLUE "  PID: $NEW_PID"
    echo_color $BLUE "  日志: $LOG_FILE"
    echo_color $BLUE "  监控: tail -f $LOG_FILE"
    echo ""
    echo_color $YELLOW "=== 启动日志 ==="
    tail -10 "$LOG_FILE" 2>/dev/null || echo_color $RED "日志文件为空"
else
    echo_color $RED "错误：下载服务启动失败！"
    echo_color $YELLOW "=== 错误日志 ==="
    tail -20 "$LOG_FILE" 2>/dev/null || echo_color $RED "无法读取日志文件"
    rm -f "$PID_FILE"
    exit 1
fi

echo ""
echo_color $GREEN "启动完成！"
echo_color $BLUE "话题格式: ros2 topic pub /download_wake_up std_msgs/String '{data: \"{\\\"url\\\":\\\"http://example.com/file.zip\\\",\\\"subdir\\\":\\\"test\\\"}\"}'"

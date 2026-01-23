#!/bin/bash

# 可执行文件名称
PROGRAM_NAME="robot_avvtn"
# 存放PID的文件，用于停止脚本识别进程
PID_FILE="${PROGRAM_NAME}.pid"
# 日志文件
LOG_FILE="program.log"

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

# 检查并停止正在运行的程序
echo_color $YELLOW "检查并停止正在运行的程序..."
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
        
        # 清理PID文件
        rm -f "$PID_FILE"
        echo_color $GREEN "程序已停止"
    else
        # PID文件存在但进程已结束，删除无效PID文件
        rm -f "$PID_FILE"
        echo_color $YELLOW "清理无效的PID文件..."
    fi
else
    echo_color $GREEN "程序当前未运行"
fi

# 等待一小段时间确保进程完全释放资源
sleep 1

# 检查是否还有残留进程
PROCESS_COUNT=$(pgrep -f "./${PROGRAM_NAME}" | wc -l)
if [ $PROCESS_COUNT -gt 0 ]; then
    echo_color $RED "警告：发现 ${PROCESS_COUNT} 个残留进程，正在清理..."
    pkill -f "./${PROGRAM_NAME}"
    sleep 1
fi

# 备份旧日志
if [ -f "$LOG_FILE" ]; then
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    BACKUP_LOG="${LOG_FILE}.backup_${TIMESTAMP}"
    echo_color $BLUE "备份旧日志: $LOG_FILE -> $BACKUP_LOG"
    cp "$LOG_FILE" "$BACKUP_LOG"
    
    # 清空日志文件（保留文件存在）
    > "$LOG_FILE"
else
    echo_color $BLUE "创建新的日志文件: $LOG_FILE"
    touch "$LOG_FILE"
fi

# 启动程序（后台运行，输出日志到program.log）
echo_color $YELLOW "正在启动 ${PROGRAM_NAME}..."
./"${PROGRAM_NAME}" >> "$LOG_FILE" 2>&1 &
# 记录当前进程PID到文件
NEW_PID=$!
echo $NEW_PID > "$PID_FILE"

# 验证启动是否成功
sleep 2  # 增加等待时间，确保程序完全初始化
if ps -p $NEW_PID > /dev/null 2>&1; then
    echo_color $GREEN "程序 ${PROGRAM_NAME} 启动成功！"
    echo_color $BLUE "  PID: $NEW_PID"
    echo_color $BLUE "  日志: $LOG_FILE"
    echo_color $BLUE "  监控: tail -f $LOG_FILE"
    
    # 显示最后几行日志
    echo_color $YELLOW "=== 程序启动日志 ==="
    tail -10 "$LOG_FILE" 2>/dev/null || echo_color $RED "  日志文件为空或读取失败"
    
    # 检查程序是否正常运行（可选）
    echo_color $BLUE "  进程信息:"
    ps -fp $NEW_PID 2>/dev/null | grep -v "UID" || echo_color $YELLOW "  无法获取进程详细信息"
else
    echo_color $RED "错误：程序 ${PROGRAM_NAME} 启动失败！"
    echo_color $YELLOW "=== 错误日志 ==="
    tail -20 "$LOG_FILE" 2>/dev/null || echo_color $RED "  无法读取日志文件"
    rm -f "$PID_FILE"
    exit 1
fi

# 可选：启动后监控
echo_color $GREEN "启动完成！"

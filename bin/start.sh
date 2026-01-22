#!/bin/bash

# 替换为你的C++可执行文件名称
PROGRAM_NAME="robot_avvtn"
# 存放PID的文件，用于停止脚本识别进程
PID_FILE="${PROGRAM_NAME}.pid"

# 检查程序是否已运行
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if ps -p "$PID" > /dev/null 2>&1; then
        echo "错误：程序 ${PROGRAM_NAME} 已在运行（PID: $PID），无需重复启动！"
        exit 1
    else
        # PID文件存在但进程已结束，删除无效PID文件
        rm -f "$PID_FILE"
        echo "清理无效的PID文件..."
    fi
fi

# 启动程序（后台运行，输出日志到program.log）
echo "正在启动 ${PROGRAM_NAME}..."
./"${PROGRAM_NAME}" > program.log 2>&1 &
# 记录当前进程PID到文件
echo $! > "$PID_FILE"

# 验证启动是否成功
sleep 1
if ps -p $(cat "$PID_FILE") > /dev/null 2>&1; then
    echo "程序 ${PROGRAM_NAME} 启动成功（PID: $(cat "$PID_FILE")）"
else
    echo "错误：程序 ${PROGRAM_NAME} 启动失败！"
    rm -f "$PID_FILE"
    exit 1
fi
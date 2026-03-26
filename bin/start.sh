#!/bin/bash

# 固定绝对路径
APP_DIR="/home/cat/robot_avvtn/bin"
PROGRAM_NAME="robot_avvtn"
LOG_DIR="/var/log/robot_avvtn"
LOG_FILE="$LOG_DIR/robot_$(date +%Y%m%d).log"

# 创建日志目录
mkdir -p $LOG_DIR

# 进入程序目录（必须）
cd $APP_DIR

export ROS_DOMAIN_ID=50

# 后台启动 + 脱离终端（nohup 防止会话关闭导致进程退出）
nohup ./$PROGRAM_NAME >> $LOG_FILE 2>&1 &

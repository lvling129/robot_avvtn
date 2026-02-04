#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ROS2 文件下载服务
订阅 download_wake_up 话题，根据接收到的URL下载文件到指定目录
支持并发下载多个文件，当所有文件下载完成后打印成功日志
"""

import os
import sys
import json
import logging
import asyncio
import aiohttp
import signal
from pathlib import Path
from datetime import datetime
from threading import Lock
from typing import List
from urllib.parse import urlparse, unquote

import rclpy
from rclpy.node import Node
from std_msgs.msg import String

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger('DownloadService')

# 获取项目根目录（脚本在 scripts/download/ 下，项目根为上级的上级）
SCRIPT_DIR = Path(__file__).parent.absolute()
PROJECT_ROOT = SCRIPT_DIR.parent.parent
DOWNLOAD_BASE_DIR = PROJECT_ROOT / 'downloads'


class DownloadTask:
    """下载任务"""
    def __init__(self, url: str, subdir: str, filename: str = None):
        self.url = url
        self.subdir = subdir
        self.filename = filename or self._extract_filename(url)
        self.status = 'pending'  # pending, downloading, completed, failed
        self.error = None
        self.start_time = None
        self.end_time = None

    def _extract_filename(self, url: str) -> str:
        """从URL中提取文件名"""
        parsed = urlparse(url)
        path = unquote(parsed.path)
        filename = os.path.basename(path)
        if not filename:
            # 如果URL没有文件名，使用时间戳
            filename = f"download_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        return filename

    @property
    def save_path(self) -> Path:
        """获取保存路径"""
        return DOWNLOAD_BASE_DIR / self.subdir / self.filename


class DownloadManager:
    """下载管理器 - 管理并发下载任务"""
    
    def __init__(self, expected_count: int = 3):
        self.expected_count = expected_count  # 期望下载的文件数量
        self.tasks: List[DownloadTask] = []
        self.lock = Lock()
        self.loop = None
        self._reset_batch()

    def _reset_batch(self):
        """重置当前批次"""
        with self.lock:
            self.tasks.clear()
            self.batch_start_time = None

    def add_task(self, url: str, subdir: str, filename: str = None) -> DownloadTask:
        """添加下载任务"""
        task = DownloadTask(url, subdir, filename)
        with self.lock:
            if self.batch_start_time is None:
                self.batch_start_time = datetime.now()
            self.tasks.append(task)
            logger.info(f"添加下载任务: {task.filename} -> {task.subdir}/")
            current_count = len(self.tasks)
        
        # 如果达到预期数量，开始批量下载
        if current_count >= self.expected_count:
            self._start_batch_download()
        
        return task

    def _start_batch_download(self):
        """开始批量下载"""
        if self.loop is None:
            return
        
        with self.lock:
            tasks_to_download = list(self.tasks)
        
        if tasks_to_download:
            logger.info(f"开始批量下载 {len(tasks_to_download)} 个文件...")
            asyncio.run_coroutine_threadsafe(
                self._download_all(tasks_to_download),
                self.loop
            )

    async def _download_all(self, tasks: List[DownloadTask]):
        """并发下载所有文件"""
        async with aiohttp.ClientSession() as session:
            download_coroutines = [self._download_file(session, task) for task in tasks]
            results = await asyncio.gather(*download_coroutines, return_exceptions=True)
        
        # 检查所有任务的状态
        success_count = sum(1 for task in tasks if task.status == 'completed')
        failed_count = sum(1 for task in tasks if task.status == 'failed')
        
        if success_count == len(tasks):
            logger.info("=" * 60)
            logger.info(f"🎉 所有 {len(tasks)} 个文件下载成功！")
            logger.info("=" * 60)
            for task in tasks:
                logger.info(f"  ✓ {task.filename} -> {task.save_path}")
        else:
            logger.warning(f"下载完成: 成功 {success_count} 个, 失败 {failed_count} 个")
            for task in tasks:
                if task.status == 'completed':
                    logger.info(f"  ✓ {task.filename}")
                else:
                    logger.error(f"  ✗ {task.filename}: {task.error}")
        
        # 重置批次，准备下一轮
        self._reset_batch()

    async def _download_file(self, session: aiohttp.ClientSession, task: DownloadTask):
        """下载单个文件"""
        task.status = 'downloading'
        task.start_time = datetime.now()
        
        try:
            # 确保目标目录存在
            save_dir = task.save_path.parent
            save_dir.mkdir(parents=True, exist_ok=True)
            
            logger.info(f"开始下载: {task.url}")
            
            async with session.get(task.url, timeout=aiohttp.ClientTimeout(total=300)) as response:
                if response.status != 200:
                    raise Exception(f"HTTP错误: {response.status}")
                
                # 写入文件
                with open(task.save_path, 'wb') as f:
                    async for chunk in response.content.iter_chunked(8192):
                        f.write(chunk)
            
            task.status = 'completed'
            task.end_time = datetime.now()
            duration = (task.end_time - task.start_time).total_seconds()
            file_size = task.save_path.stat().st_size / 1024  # KB
            logger.info(f"下载完成: {task.filename} ({file_size:.1f} KB, {duration:.1f}秒)")
            
        except Exception as e:
            task.status = 'failed'
            task.error = str(e)
            task.end_time = datetime.now()
            logger.error(f"下载失败: {task.filename} - {e}")


class DownloadServiceNode(Node):
    """ROS2 下载服务节点"""
    
    def __init__(self, download_manager: DownloadManager):
        super().__init__('download_service_node')
        self.download_manager = download_manager
        
        # 订阅 download_wake_up 话题
        self.subscription = self.create_subscription(
            String,
            'download_wake_up',
            self.download_callback,
            10
        )
        
        logger.info("下载服务节点已启动")
        logger.info(f"订阅话题: download_wake_up")
        logger.info(f"下载目录: {DOWNLOAD_BASE_DIR}")

    def download_callback(self, msg: String):
        """处理下载请求"""
        try:
            # 解析JSON消息
            # 预期格式: {"url": "http://...", "subdir": "subdir_name", "filename": "optional_filename"}
            data = json.loads(msg.data)
            
            url = data.get('url')
            subdir = data.get('subdir', '')
            filename = data.get('filename')  # 可选
            
            if not url:
                logger.error("无效的下载请求: 缺少url字段")
                return
            
            logger.info(f"收到下载请求: url={url}, subdir={subdir}")
            
            # 添加下载任务
            self.download_manager.add_task(url, subdir, filename)
            
        except json.JSONDecodeError as e:
            logger.error(f"JSON解析错误: {e}, 原始数据: {msg.data}")
        except Exception as e:
            logger.error(f"处理下载请求时出错: {e}")


async def async_main(download_manager: DownloadManager):
    """异步主函数"""
    download_manager.loop = asyncio.get_event_loop()
    
    # 保持异步循环运行
    while rclpy.ok():
        await asyncio.sleep(0.1)


def main():
    """主函数"""
    logger.info("=" * 60)
    logger.info("ROS2 文件下载服务启动")
    logger.info("=" * 60)
    
    # 确保下载目录存在
    DOWNLOAD_BASE_DIR.mkdir(parents=True, exist_ok=True)
    
    # 初始化ROS2
    rclpy.init()
    
    # 创建下载管理器
    download_manager = DownloadManager(expected_count=3)
    
    # 创建节点
    node = DownloadServiceNode(download_manager)
    
    # 设置信号处理
    def signal_handler(sig, frame):
        logger.info("收到停止信号，正在关闭...")
        rclpy.shutdown()
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        # 在单独的线程中运行异步事件循环
        import threading
        
        loop = asyncio.new_event_loop()
        download_manager.loop = loop
        
        def run_async_loop():
            asyncio.set_event_loop(loop)
            loop.run_forever()
        
        async_thread = threading.Thread(target=run_async_loop, daemon=True)
        async_thread.start()
        
        # ROS2 spin
        logger.info("下载服务正在运行，等待下载请求...")
        rclpy.spin(node)
        
    except KeyboardInterrupt:
        logger.info("收到键盘中断")
    except Exception as e:
        logger.error(f"运行时错误: {e}")
    finally:
        node.destroy_node()
        rclpy.shutdown()
        logger.info("下载服务已关闭")


if __name__ == '__main__':
    main()

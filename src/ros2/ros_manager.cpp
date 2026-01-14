#include "ros_manager.hpp"
#include "utils/Logger.hpp"

ROSManager& ROSManager::getInstance() {
    static ROSManager instance;
    return instance;
}

void ROSManager::init(int argc, char const *argv[]) {
    if (initialized_) return;

    // 初始化ROS2
    rclcpp::init(argc, argv);

    // 创建节点
    node_ = std::make_shared<rclcpp::Node>("robot_avvtn_node");

    // 创建发布器
    log_publisher_ = node_->create_publisher<std_msgs::msg::String>("robot_avvtn_log", 10);
    chat_history_publisher_ = node_->create_publisher<std_msgs::msg::String>("robot_avvtn_chat_history", 10);
    status_publisher_ = node_->create_publisher<std_msgs::msg::String>("robot_avvtn_status", 10);
    
    LOG_INFO("ROS管理器初始化成功");
    initialized_ = true;

    // 创建ROS spin线程
    ros_spin_thread_ = std::thread([this]() {
        LOG_INFO("ROS2回调线程启动");
        rclcpp::spin(node_);
        LOG_INFO("ROS2回调线程结束");
    });
}

std::shared_ptr<rclcpp::Node> ROSManager::getNode() {
    return node_;
}

void ROSManager::publishLog(const std::string& log_msg) {
    if (!initialized_) return;

    auto message = std_msgs::msg::String();
    message.data = log_msg;
    log_publisher_->publish(message);
}

void ROSManager::publishChatHistory(const std::string& chat_msg) {
    if (!initialized_) return;

    auto message = std_msgs::msg::String();
    message.data = chat_msg;
    chat_history_publisher_->publish(message);
}

void ROSManager::publishStatus(const std::string& status_msg) {
    if (!initialized_) return;

    auto message = std_msgs::msg::String();
    message.data = status_msg;
    status_publisher_->publish(message);
}

void ROSManager::subscribeTopic(const std::string& topic_name,
                               std::function<void(const std_msgs::msg::String::SharedPtr)> callback) {
    if (!initialized_) return;
    
    auto subscriber = node_->create_subscription<std_msgs::msg::String>(
        topic_name, 10, callback);
    
    LOG_INFO("已订阅话题: {}", topic_name);
}

void ROSManager::shutdown() {
    if (!initialized_) return;

    rclcpp::shutdown();
    if (ros_spin_thread_.joinable()) {
        ros_spin_thread_.join();
    }

    initialized_ = false;
    LOG_INFO("ROS管理器已关闭");
}

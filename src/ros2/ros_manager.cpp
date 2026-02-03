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
    chat_history_nostream_publisher_ = node_->create_publisher<std_msgs::msg::String>("robot_avvtn_chat_history_nostream", 10);
    wakeup_detail_publisher_ = node_->create_publisher<std_msgs::msg::String>("avvtn_wake", 10);

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
/*
    STATUS_WAITING_CONNECTION
    STATUS_WAITING_WAKEUP
    STATUS_IN_CONVERSATION
    STATUS_WAITING_CONVERSATION
*/
void ROSManager::publishStatus(const std::string& status_msg) {
    if (!initialized_) return;

    auto message = std_msgs::msg::String();
    message.data = status_msg;
    status_publisher_->publish(message);
}

void ROSManager::publishChatHistoryNoStream(const std::string& status_msg) {
    if (!initialized_) return;

    auto message = std_msgs::msg::String();
    message.data = status_msg;
    chat_history_nostream_publisher_->publish(message);
}

void ROSManager::publishWakeupDetail(const std::string& status_msg) {
    if (!initialized_) return;

    auto message = std_msgs::msg::String();
    message.data = status_msg;
    wakeup_detail_publisher_->publish(message);
}

void ROSManager::subscribeTopic(const std::string& topic_name,
                               std::function<void(const std_msgs::msg::String::SharedPtr)> callback) {
    if (!initialized_) return;
    

    // 检查是否已经订阅了该话题
    if (subscribers_.find(topic_name) != subscribers_.end()) {
        LOG_WARN("已经订阅了话题: {}", topic_name);
        return;
    }

    // 创建订阅者并保存
    auto subscriber = node_->create_subscription<std_msgs::msg::String>(
        topic_name, 10, callback);
    
    // 保存到map中，确保订阅者不被销毁
    subscribers_[topic_name] = subscriber;
    
    LOG_INFO("已订阅话题: {%s}", topic_name.c_str());
    LOG_INFO("当前订阅话题数量: {%d}", subscribers_.size());
}

void ROSManager::shutdown() {
    if (!initialized_) return;

    // 清理订阅者
    subscribers_.clear();

    rclcpp::shutdown();
    if (ros_spin_thread_.joinable()) {
        ros_spin_thread_.join();
    }

    initialized_ = false;
    LOG_INFO("ROS管理器已关闭");
}

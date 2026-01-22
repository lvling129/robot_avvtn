#ifndef ROS_MANAGER_HPP
#define ROS_MANAGER_HPP

#include <memory>
#include <string>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class ROSManager {
public:
    // 获取单例实例
    static ROSManager& getInstance();
    
    // 初始化ROS管理器（在main函数中调用）
    void init(int argc, char const *argv[]);
    
    // 获取节点
    std::shared_ptr<rclcpp::Node> getNode();
    
    // 发布日志
    void publishLog(const std::string& log_msg);
    
    // 发布聊天历史
    void publishChatHistory(const std::string& chat_msg);
    
    // 发布状态
    void publishStatus(const std::string& status_msg);

    // 发布聊天历史(非流式文本)
    void publishChatHistoryNoStream(const std::string& chat_msg);
    
    // 订阅话题
    void subscribeTopic(const std::string& topic_name, 
                       std::function<void(const std_msgs::msg::String::SharedPtr)> callback);
    
    // 关闭ROS
    void shutdown();
    
private:
    ROSManager() = default;
    ~ROSManager() = default;
    
    // 禁用拷贝和赋值
    ROSManager(const ROSManager&) = delete;
    ROSManager& operator=(const ROSManager&) = delete;
    
    std::shared_ptr<rclcpp::Node> node_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr log_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr chat_history_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr chat_history_nostream_publisher_;
    
    bool initialized_ = false;
    std::thread ros_spin_thread_;
};

#endif // ROS_MANAGER_HPP

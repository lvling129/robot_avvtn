#include "avvtn_capture.h"
#include "utils/Logger.hpp"
#include "ros2/ros_manager.hpp"
#include "utils/json.hpp"



void AvvtnCapture::handleSkill(const std::string& text_str)
{
    try {
        // 解析text字段中的JSON
        auto text_root = nlohmann::json::parse(text_str);

        if (text_root.contains("category") && text_root["category"].is_string()) {
            std::string category = text_root["category"].get<std::string>();
            if (category == "IFLYTEK.datetimePro")
            {
                /*官方时间技能*/
                return;
            }
        }

        // 检查data字段是否存在
        if (!text_root.contains("data") || !text_root["data"].is_object()) {
            LOG_WARN("JSON中缺少data字段或data不是对象类型");
            return;
        }

        auto& data = text_root["data"];

        // 检查type字段是否存在
        if (data.contains("type") && data["type"].is_string()) {
            std::string type = data["type"].get<std::string>();
            if (type == "face_rec_start")
            {
                LOG_INFO("执行调用人脸识别服务意图!!!");
            }
            else if (type == "move")
            {
                LOG_INFO("执行移动意图!!!");
            }
            else if (type == "turn")
            {
                LOG_INFO("执行转向意图!!!");
            }
            else if (type == "action")
            {
                LOG_INFO("执行动作意图!!!");
            }
            else if (type == "stop")
            {
                LOG_INFO("执行停止意图!!!");
            }
            else if (type == "shut_up")
            {
                LOG_INFO("执行停止语音交互意图!!!");
            }
            else if (type == "vip")
            {
                LOG_INFO("执行 VIP 场景意图!!!");
            }
            else if (type == "new_year")
            {
                LOG_INFO("执行通用祝福意图!!!");
            }
            else
            {
                LOG_INFO("新的未定义技能！！！");
            }
        }

    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("解析text字段失败: %s, text_str: %s", e.what(), text_str.c_str());
        return;
    } catch (const nlohmann::json::type_error& e) {
        LOG_ERROR("类型错误: %s", e.what());
        return;
    }

}
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

        // 检查type字段是否存在
        if (text_root.contains("data") &&
            text_root["data"].contains("result") &&
            text_root["data"]["result"].is_array() &&
            text_root["data"]["result"].size() > 0 &&
            text_root["data"]["result"][0].contains("type") &&
            text_root["data"]["result"][0]["type"].is_string())

        {
            LOG_INFO("捕获到技能type!!!");
            std::string type = text_root["data"]["result"][0]["type"].get<std::string>();

            if (type == "face_rec_start")
            {
                LOG_INFO("执行调用人脸识别服务意图!!!");
            }
            else if (type == "move")
            {
                LOG_INFO("执行移动意图!!!");

                // 提取移动参数
                auto& result = text_root["data"]["result"][0];
                std::string intent_name = result["intentName"].get<std::string>();
                int id = result["id"].get<std::int32_t>();

                // 提取slots
                std::string distance;
                std::string move_unit;

                if (result.contains("slots") && result["slots"].is_object()) {
                    auto& slots = result["slots"];
                    
                    if (slots.contains("distance") && 
                        slots["distance"].is_object() &&
                        slots["distance"].contains("normValue") &&
                        slots["distance"]["normValue"].is_string())
                    {
                        distance = slots["distance"]["normValue"].get<std::string>();
                    }

                    if (slots.contains("move_unit") && 
                        slots["move_unit"].is_object() &&
                        slots["move_unit"].contains("normValue") &&
                        slots["move_unit"]["normValue"].is_string())
                    {
                        move_unit = slots["move_unit"]["normValue"].get<std::string>();
                    }
                }
                
                LOG_INFO("移动参数: intent_name=%s, id=%d, distance=%s, move_unit=%s",
                        intent_name.c_str(), id, distance.c_str(), move_unit.c_str());
                
                // TODO: 播放音频文件
                // TODO: 发送移动请求
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

        // 检查voice_answer content字段是否存在
        if (text_root.contains("voice_answer") &&
            text_root["voice_answer"].is_array() &&
            text_root["voice_answer"].size() > 0 &&
            text_root["voice_answer"][0].contains("content") &&
            text_root["voice_answer"][0]["content"].is_string() &&
            text_root["voice_answer"][0].contains("type") &&
            text_root["voice_answer"][0]["type"] == "TTS")

        {
            std::string voice_answer_content = text_root["voice_answer"][0]["content"].get<std::string>();
            LOG_INFO("技能返回TTS内容文本: %s", voice_answer_content.c_str());
            ignore_tts_sid_ = current_iat_sid_;
            // 调用语音合成TTS
            aiui_wrapper_.StartTTS(voice_answer_content);
        }


    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("解析text字段失败: %s, text_str: %s", e.what(), text_str.c_str());
        return;
    } catch (const nlohmann::json::type_error& e) {
        LOG_ERROR("类型错误: %s", e.what());
        return;
    }

}
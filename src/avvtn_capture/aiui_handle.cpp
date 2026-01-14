#include "avvtn_capture.h"
#include "utils/Logger.hpp"
#include "ros2/ros_manager.hpp"

void AvvtnCapture::aiuiCallback(void *user_data, const IAIUIEvent &event)
{
    AvvtnCapture *self = static_cast<AvvtnCapture *>(user_data);
    try
    {
        switch (event.getEventType())
        {
            // SDK状态
            case AIUIConstant::EVENT_STATE:
            {
                LOG_INFO("接收到AIUI SDK状态信息");
                switch (event.getArg1())
                {
                    case AIUIConstant::STATE_IDLE: 
                        std::cout << "EVENT_STATE: STATE_IDLE" << std::endl;
                        LOG_INFO("AIUI当前状态: IDLE");
                        ROSManager::getInstance().publishStatus("当前状态: IDLE");
                        break;
                    case AIUIConstant::STATE_READY:
                        std::cout << "EVENT_STATE: STATE_READY" << std::endl;
                        LOG_INFO("AIUI当前状态: READY");
                        ROSManager::getInstance().publishStatus("当前状态: READY");
                        break;
                    case AIUIConstant::STATE_WORKING:
                        std::cout << "EVENT_STATE: STATE_WORKING" << std::endl;
                        LOG_INFO("AIUI当前状态: WORKING");
                        ROSManager::getInstance().publishStatus("当前状态: WORKING");
                        break;
                }
            }
            break;

            // 唤醒事件
            case AIUIConstant::EVENT_WAKEUP:
            {
                ROSManager::getInstance().publishStatus("已唤醒");
                LOG_INFO("接收到AIUI唤醒事件EVENT_WAKEUP: %s", event.getInfo());
                LOG_INFO("pcm播放器停止播放");
                std::cout << "EVENT_WAKEUP: " << event.getInfo() << std::endl;
                aiui_pcm_player_stop();
            }
            break;

            // 休眠事件
            case AIUIConstant::EVENT_SLEEP:
            {
                ROSManager::getInstance().publishStatus("休眠中，等待唤醒");
                LOG_INFO("接收到AIUI休眠事件EVENT_SLEEP: arg1 = %d", event.getArg1());
                std::cout << "EVENT_SLEEP: arg1=" << event.getArg1() << std::endl;
            }
            break;

            // VAD事件
            case AIUIConstant::EVENT_VAD:
            {
                LOG_DEBUG("接收到AIUI VAD事件");
                switch (event.getArg1())
                {
                    case AIUIConstant::VAD_BOS_TIMEOUT:
                        std::cout << "EVENT_VAD: VAD_BOS_TIMEOUT" << std::endl;
                        LOG_DEBUG("EVENT_VAD: VAD_BOS_TIMEOUT");
                        break;
                    case AIUIConstant::VAD_BOS:
                        std::cout << "EVENT_VAD: BOS" << std::endl;
                        LOG_DEBUG("EVENT_VAD: BOS");
                        break;
                    case AIUIConstant::VAD_EOS:
                        std::cout << "EVENT_VAD: EOS" << std::endl;
                        LOG_DEBUG("EVENT_VAD: EOS");
                        break;
                    case AIUIConstant::VAD_VOL:
                        //LOG_DEBUG("EVENT_VAD: vol = %d", event.getArg2());
                        // cout << "EVENT_VAD: vol=" << event.getArg2() << endl;
                        break;
                }
            }
            break;

            // 结果事件
            case AIUIConstant::EVENT_RESULT:
            {
                Json::Value bizParamJson;
                Json::Reader reader;
                if (!reader.parse(event.getInfo(), bizParamJson, false))
                {
                    LOG_ERROR("parse error! info = %s", event.getInfo());
                    std::cout << "parse error! info=" << event.getInfo() << std::endl;
                    break;
                }
                Json::Value &data    = (bizParamJson["data"])[0];
                Json::Value &params  = data["params"];
                Json::Value &content = (data["content"])[0];

                std::string sub = params["sub"].asString();
                std::string sid = event.getData()->getString("sid", "");

                if (sub == "iat")
                {
                    if (sid != self->current_iat_sid_)
                    {
                        LOG_DEBUG("iat**********************************");
                        LOG_DEBUG("sid = %s", sid.c_str());
                        std::cout << "**********************************" << std::endl;
                        std::cout << "sid=" << sid << std::endl;
                        self->current_iat_sid_ = sid;

                        LOG_DEBUG("新的会话，清空之前识别缓存，并且停止播放");
                        // 新的会话，清空之前识别缓存，并且停止播放
                        self->iat_text_buffer_.clear();
                        self->stream_nlp_answer_buffer_.clear();
                        self->aiui_wrapper_.listener_->tts_helper_ptr_->clear();
                        self->intent_cnt_ = 0;
                    }
                }
                else if (sub == "tts")
                {
                    if (sid != self->current_tts_sid_)
                    {
                        LOG_DEBUG("tts**********************************");
                        LOG_DEBUG("sid = %s", sid.c_str());
                        std::cout << "**********************************" << std::endl;
                        std::cout << "sid=" << sid << std::endl;
                        self->tts_len_         = 0;
                        self->current_tts_sid_ = sid;
                    }
                }
                int dataLen = 0;
                Json::Value empty;
                std::string cnt_id = content.get("cnt_id", empty).asString();
                // 注意：当buffer里存字符串时也不是以0结尾，当使用C语言时，转成字符串则需要自已在末尾加0
                const char *buffer = event.getData()->getBinary(cnt_id.c_str(), &dataLen);

                std::string resultStr = std::string(buffer, dataLen);
                if (sub != "tts")
                {
                    LOG_DEBUG("JSON原始数据: %s", resultStr.c_str());
                }
                // 当前仅解析iat tts nlp
                if (sub == "iat")
                {
                    LOG_DEBUG("接收到AIUI返回的语音识别结果iat");
                    LOG_DEBUG("%s: ", event.getInfo());

                    self->handleAiuiIat(reader, buffer, dataLen);
                }
                else if (sub == "tts")
                {
                    LOG_DEBUG("接收到AIUI返回的语音合成结果tts");
                    self->handleAiuiTts(reader, content, event, bizParamJson, buffer, dataLen);
                }
                else if (sub == "nlp")
                {
                    LOG_DEBUG("接收到AIUI返回的语义理解结果nlp");
                    LOG_DEBUG("%s: ", event.getInfo());
                    LOG_DEBUG("%s", resultStr.c_str());

                    self->handleAiuiStreamNlp(reader, buffer, dataLen);
                }
                else if (sub == "event")
                {
                    //服务事件
                    LOG_DEBUG("接收到AIUI返回的【服务事件event】");
                }
                else if (sub == "cbm_tidy")
                {
                    //语义规整
                    LOG_INFO("接收到AIUI返回的【语义规整cbm_tidy】");
                    Json::Value root;
                    Json::Reader reader;
                    if (!reader.parse(resultStr, root)) {
                        LOG_ERROR("解析失败: %s",reader.getFormattedErrorMessages().c_str());
                        break;
                    }
                    Json::Value cbm_tidy = root["cbm_tidy"];
                    if (cbm_tidy.isMember("text") && cbm_tidy["text"].isString())
                    {
                        std::string text_str = cbm_tidy["text"].asString();
                        Json::Value text_root;
                        Json::Reader text_reader;
                        if (!text_reader.parse(text_str, text_root))
                        {
                            LOG_ERROR("解析失败: %s",reader.getFormattedErrorMessages().c_str());
                            break;
                        }
                        // 解析intent数组
                        if (text_root.isMember("intent") && text_root["intent"].isArray()) {
                            Json::Value& intentArray = text_root["intent"];
                            for (Json::ArrayIndex i = 0; i < intentArray.size(); i++)
                            {
                                LOG_INFO("语义规整结果%d: %s", intentArray[i]["index"].asInt(), intentArray[i]["value"].asString().c_str());
                            }
                        }
                    }
                }
                else if (sub == "cbm_semantic")
                {
                    //传统语义技能
                    LOG_INFO("接收到AIUI返回的【传统语义技能cbm_semantic】");
                    Json::Value root;
                    Json::Reader reader;
                    if (!reader.parse(resultStr, root)) {
                        LOG_ERROR("解析失败: %s",reader.getFormattedErrorMessages().c_str());
                        break;
                    }
                    Json::Value cbm_semantic = root["cbm_semantic"];
                    if (cbm_semantic.isMember("text") && cbm_semantic["text"].isString())
                    {
                        std::string text_str = cbm_semantic["text"].asString();
                        Json::Value text_root;
                        Json::Reader text_reader;
                        if (text_reader.parse(text_str, text_root))
                        {
                            if (text_root.isMember("rc") && text_root["rc"].isInt()) {
                                int rc = text_root["rc"].asInt();
                                if (rc != 0)
                                {
                                    LOG_INFO("技能结果：未命中技能");
                                    break;
                                }
                                else
                                {
                                    LOG_INFO("技能结果：命中技能");
                                }
                            }

                            if (text_root.isMember("text") && text_root["text"].isString()) {
                                std::string text = text_root["text"].asString();
                                LOG_INFO("技能返回内容: %s", text.c_str());
                            }

                            if (text_root.isMember("version") && text_root["version"].isString()) {
                                std::string version = text_root["version"].asString();
                                LOG_INFO("技能版本: %s", version.c_str());
                            }

                            if (text_root.isMember("service") && text_root["service"].isString()) {
                                std::string service = text_root["service"].asString();
                                LOG_INFO("技能名称: %s", service.c_str());
                            }
                        }
                    }
                }
                else if (sub == "cbm_tool_pk")
                {
                    //意图落域
                    LOG_INFO("接收到AIUI返回的【意图落域cbm_tool_pk】");
                    Json::Value root;
                    Json::Reader reader;
                    bool success = reader.parse(resultStr, root);
                    if (!success) {
                        LOG_ERROR("解析失败: %s",reader.getFormattedErrorMessages().c_str());
                        break;
                    }
                    Json::Value cbm_tool_pk = root["cbm_tool_pk"];
                    if (cbm_tool_pk.isMember("text") && cbm_tool_pk["text"].isString())
                    {
                        std::string text_str = cbm_tool_pk["text"].asString();
                        Json::Value text_root;
                        Json::Reader text_reader;
                        if (text_reader.parse(text_str, text_root))
                        {
                            if (text_root.isMember("pk_type") && text_root["pk_type"].isString()) {
                                std::string pk_type = text_root["pk_type"].asString();
                                LOG_INFO("落域结果判定来源模块: %s", pk_type.c_str());
                            }
                            
                            if (text_root.isMember("pk_source") && text_root["pk_source"].isString())
                            {
                                std::string pk_source_text_str = text_root["pk_source"].asString();
                                Json::Value pk_source_root;
                                Json::Reader pk_source_reader;
                                if (pk_source_reader.parse(pk_source_text_str, pk_source_root))
                                {
                                    if (pk_source_root.isMember("domain") && pk_source_root["domain"].isString())
                                    {
                                        std::string domain = pk_source_root["domain"].asString();
                                        LOG_INFO("落域结果: %s", domain.c_str());
                                    }
                                }
                            }
                        }
                    }
                }
                else if (sub == "cbm_retrieval_classify")
                {
                    LOG_INFO("JSON原始数据: %s", resultStr.c_str());
                    //知识分类
                    LOG_INFO("接收到AIUI返回的【知识分类cbm_retrieval_classify】");
                    Json::Value root;
                    Json::Reader reader;
                    if (!reader.parse(resultStr, root)) {
                        LOG_ERROR("解析失败: %s",reader.getFormattedErrorMessages().c_str());
                        break;
                    }
                    Json::Value cbm_retrieval_classify = root["cbm_retrieval_classify"];
                    if (cbm_retrieval_classify.isMember("text") && cbm_retrieval_classify["text"].isString())
                    {
                        std::string text_str = cbm_retrieval_classify["text"].asString();
                        Json::Value text_root;
                        Json::Reader text_reader;
                        if (text_reader.parse(text_str, text_root))
                        {
                            if (text_root.isMember("type") && text_root["type"].isInt()) {
                                int type = text_root["type"].asInt();
                                if (type == 0)
                                {
                                    LOG_INFO("不走知识查询或联网搜索");
                                }
                                else
                                {
                                    LOG_INFO("走知识查询或联网搜索");
                                }
                            }
                        }
                    }
                }
                else if (sub == "cbm_plugin")
                {
                    //智能体
                    LOG_INFO("接收到AIUI返回的【智能体cbm_plugin】");
                }
                else if (sub == "cbm_knowledge")
                {
                    LOG_INFO("JSON原始数据: %s", resultStr.c_str());
                    //知识溯源
                    LOG_INFO("接收到AIUI返回的【知识溯源cbm_knowledge】");
                    Json::Value root;
                    Json::Reader reader;
                    bool success = reader.parse(resultStr, root);
                    if (!success) {
                        LOG_ERROR("解析失败: %s",reader.getFormattedErrorMessages().c_str());
                        break;
                    }
                    Json::Value cbm_knowledge = root["cbm_knowledge"];

                    if (cbm_knowledge.isMember("text") && cbm_knowledge["text"].isString())
                    {
                        std::string text_str = cbm_knowledge["text"].asString();
                        Json::Value text_array;
                        Json::Reader text_reader;
                        if (text_reader.parse(text_str, text_array))
                        {
                            if (text_array.size() == 0)
                            {
                                LOG_INFO("未命中知识库");
                            }
                            for (int i = 0; i < text_array.size(); i++)
                            {
                                Json::Value item = text_array[i];
                                
                                std::cout << std::endl;
                                if (item.isMember("score") && item["score"].isDouble())
                                {
                                    std::double_t score = item["score"].asDouble();
                                    LOG_INFO("score: %lf", score);
                                }
                                if (item.isMember("repoId") && item["repoId"].isString())
                                {
                                    std::string repoId = item["repoId"].asString();
                                    LOG_INFO("知识库Id: %s", repoId.c_str());
                                }
                                if (item.isMember("docName") && item["docName"].isString())
                                {
                                    std::string docName = item["docName"].asString();
                                    LOG_INFO("来源文档: %s", docName.c_str());
                                }
                                if (item.isMember("repoName") && item["repoName"].isString())
                                {
                                    std::string repoName = item["repoName"].asString();
                                    LOG_INFO("repo名字: %s", repoName.c_str());
                                }
                                if (item.isMember("content") && item["content"].isString())
                                {
                                    std::string content = item["content"].asString();
                                    LOG_INFO("内容: %s", content.c_str());
                                }
                            }
                        }
                    }
                    else
                    {
                        LOG_INFO("未命中知识库");
                    }
                }
                else
                {
                    // 其他结果
                    //std::cout << sub << ": " << event.getInfo() << std::endl;
                    //std::cout << resultStr << std::endl;
                    
                    //LOG_INFO("除iat,tts,nlp外的其他返回数据:");
                    //LOG_INFO("%s: ", event.getInfo());
                    //LOG_INFO("%s", resultStr.c_str());
                }
            }
            break;

            // 命令返回事件
            case AIUIConstant::EVENT_CMD_RETURN:
            {
                LOG_INFO("AIUI执行完命令返回");
                LOG_INFO("CMD = %d, retCode = %d", event.getArg1(), event.getArg2());
                if (AIUIConstant::SUCCESS == event.getArg2())
                {
                    LOG_INFO("AIUI命令执行成功");
                }
                else
                {
                    LOG_ERROR("AIUI命令执行失败");
                }
                if (AIUIConstant::CMD_SYNC == event.getArg1())
                {
                    int retCode = event.getArg2();
                    if (AIUIConstant::SUCCESS == retCode)
                    {
                        std::string sid = event.getData()->getString("sid", "");
                        std::cout << "数据同步成功, sid=" << sid << std::endl;
                    }
                    else
                    {
                        std::cout << "数据同步失败, 错误码=" << retCode << std::endl;
                    }
                }
            }
            break;

            // 开始录音事件
            case AIUIConstant::EVENT_START_RECORD:
            {
                LOG_INFO("AIUI已开始录音");
                std::cout << "EVENT_START_RECORD" << std::endl;
            }
            break;

            // 停止录音事件
            case AIUIConstant::EVENT_STOP_RECORD:
            {
                LOG_INFO("AIUI已停止录音");
                std::cout << "EVENT_STOP_RECORD" << std::endl;
            }
            break;

            // 出错事件
            case AIUIConstant::EVENT_ERROR:
            {
                std::ostringstream oss;
                oss << "AIUI出错EVENT_ERROR: error = " << event.getArg1() 
                    << ", des = " << event.getInfo();
                std::string error_msg = oss.str();
                ROSManager::getInstance().publishStatus(error_msg);
                LOG_ERROR("AIUI出错EVENT_ERROR: error = %d, des = %s", event.getArg1(), event.getInfo());
                std::cout << "EVENT_ERROR: error=" << event.getArg1() << ", des=" << event.getInfo() << std::endl;
            }
            break;

            // 连接到服务器
            case AIUIConstant::EVENT_CONNECTED_TO_SERVER:
            {
                ROSManager::getInstance().publishStatus("已连接到服务器");
                std::string uid = event.getData()->getString("uid", "");
                std::cout << "EVENT_CONNECTED_TO_SERVER, uid=" << uid << std::endl;
                LOG_INFO("已连接到服务器, uid = %s", uid.c_str());
            }
            break;

            // 与服务器断开连接
            case AIUIConstant::EVENT_SERVER_DISCONNECTED:
            {
                ROSManager::getInstance().publishStatus("与服务器断开");
                LOG_INFO("与AIUI服务器断开");
                std::cout << "EVENT_SERVER_DISCONNECTED" << std::endl;
            }
            break;

            default:
                break;
        }
    }
    catch (std::exception &e)
    {
        std::cout << "Exception in callback: " << e.what() << std::endl;
        LOG_ERROR("Exception in callback: %s", e.what());
    }
    return;
}

void AvvtnCapture::handleAiuiIat(Json::Reader &reader, const char *buffer, int len)
{
    // 语音识别结果
    std::string resultStr = std::string(buffer, len);    // 注意：这里不能用string resultStr = buffer，因为buffer不一定以0结尾
    Json::Value resultJson;
    if (reader.parse(resultStr, resultJson, false))
    {
        Json::Value textJson = resultJson["text"];
        bool isWpgs          = false;
        if (textJson.isMember("pgs"))
        {
            isWpgs = true;
        }

        if (isWpgs)
        {
            iat_text_buffer_ = IatResultUtil::parsePgsIatText(textJson);
        }
        else
        {
            // 结果拼接起来
            iat_text_buffer_.append(IatResultUtil::parseIatResult(textJson));
        }

        // 是否是该次会话最后一个识别结果
        bool isLast = textJson["ls"].asBool();
        if (isLast)
        {
            /*发送ROS2话题robot_avvtn_chat_history  问*/
            std::ostringstream oss;
            oss << "Q: " << iat_text_buffer_; 
            std::string ask_msg = oss.str();
            ROSManager::getInstance().publishChatHistory(ask_msg);

            LOG_INFO("IAT语音识别结果: %s", iat_text_buffer_.c_str());
            std::cout << "iat: " << iat_text_buffer_ << std::endl;
            aiui_wrapper_.StartTTS(iat_text_buffer_);
            iat_text_buffer_.clear();
        }
    }
}

void AvvtnCapture::handleAiuiTts(const Json::Reader &reader, const Json::Value content, const IAIUIEvent event, Json::Value bizParamJson, const char *buffer, int len)
{
    Json::Value empty;
    // 语音合成结果，返回url或者pcm音频
    Json::Value &&isUrl = content.get("url", empty);
    if (isUrl.asString() == "1")
    {
        // 云端返回的是url链接，可以用播放器播放
        std::cout << "tts_url=" << std::string(buffer, len) << std::endl;
        LOG_DEBUG("云端返回的是url链接 tts_url = %s", std::string(buffer, len).c_str());
    }
    else
    {
        // 云端返回的是pcm音频，分成一块块流式返回
        LOG_DEBUG("云端返回的是pcm音频, 分成一块块流式返回");
        int progress    = 0;
        int dts         = content["dts"].asInt();
        std::string tag = event.getData()->getString("tag", "");
        if (tag.find("stream_nlp_tts") == 0)
        {
            LOG_DEBUG("流式语义应答的合成");
            // 流式语义应答的合成
            aiui_wrapper_.listener_->tts_helper_ptr_->onOriginTtsData(tag, bizParamJson, buffer, len);
        }
        else
        {
            // 只有碰到开始块和(特殊情况:合成字符比较少时只有一包tts，dts = 2)，开启播放器
            if (dts == AIUIConstant::DTS_BLOCK_FIRST || dts == AIUIConstant::DTS_ONE_BLOCK || (dts == AIUIConstant::DTS_BLOCK_LAST && 0 == tts_len_))
            {

                if (aiui_pcm_player_get_state() != PCM_PLAYER_STATE_STARTED)
                {
                    LOG_DEBUG("开启PCM播放器");
                    int ret = aiui_pcm_player_start();
                }
            }

            tts_len_ += len;
            LOG_DEBUG("播放TTS音频");
            aiui_pcm_player_write(0, buffer, len, dts, progress);
        }
        // 若要保存合成音频，请打开以下开关
#if 1
        LOG_DEBUG("保存TTS音频到本地./tts.pcm");
        static FILE *tts_file = nullptr;
        if (tts_file == nullptr)
        {
            tts_file = fopen("./tts.pcm", "wb");
        }
        if (len > 0)
        {
            fwrite(buffer, 1, len, tts_file);
        }

#endif
    }
}

void AvvtnCapture::handleAiuiStreamNlp(Json::Reader &reader, const char *buffer, int len)
{
    // 语义理解结果
    std::string resultStr = std::string(buffer, len);

    Json::Value resultJson;
    if (reader.parse(resultStr, resultJson, false))
    {

        if (resultJson.isMember("nlp"))
        {
            // AIUI v2的语义结果
            Json::Value nlpJson = resultJson["nlp"];
            std::string text    = nlpJson["text"].asString();

            // 大模型语义结果
            // 流式nlp结果里面有seq和status字段
            int seq    = nlpJson["seq"].asInt();
            int status = nlpJson["status"].asInt();

            if (status == 0)
            {
                stream_nlp_index_ = 0;
            }
            if (status == 2 && seq == 0)
            {
                stream_nlp_index_ = 0;
            }
            /* 多意图取最后一次问题的结果进行tts合成 */
            if (intent_cnt_ > 1)
            {
                int currentIntentIndex = 0;
                Json::Value metaNlpJson;
                Json::Value textJson = resultJson["cbm_meta"].get("text", metaNlpJson);
                if (reader.parse(textJson.asString(), metaNlpJson, false))
                {
                    currentIntentIndex = metaNlpJson["nlp"]["intent"].asInt();
                    if ((intent_cnt_ - 1) != currentIntentIndex)
                    {
                        LOG_INFO("ignore nlp: %s", resultStr.c_str());
                        std::cout << "ignore nlp:" << resultStr << std::endl;
                        return;
                    }
                }
                else
                {
                    LOG_INFO("ignore nlp: %s", resultStr.c_str());
                    std::cout << "ignore nlp:" << resultStr << std::endl;
                    return;
                }
            }

#ifndef USE_POST_SEMANTIC_TTS
            // 如果使用应用的语义后合成不需要在调用下面的函数否则tts的播报会重复
            aiui_wrapper_.listener_->tts_helper_ptr_->addText(text, stream_nlp_index_++, status);
#endif

            LOG_INFO("大模型返回nlp语义结果: seq = %d, status = %d, answer（应答语）: %s", seq, status, text.c_str());
            std::cout << "seq=" << seq << ", status=" << status << ", answer（应答语）: " << text << std::endl;

            if (status == 2)
            {
                stream_nlp_answer_buffer_.clear();
            }
        }
        else
        {
            LOG_INFO("无效nlp结果");
            LOG_INFO("----------------------------------");
            LOG_INFO("nlp: %s", resultStr.c_str());
            // 无效结果，把原始结果打印出来
            std::cout << "----------------------------------" << std::endl;
            std::cout << "nlp: " << resultStr << std::endl;
        }
    }
}
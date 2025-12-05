/*
 * MCP Server Implementation
 * Reference: https://modelcontextprotocol.io/specification/2024-11-05
 */

#include "mcp_server.h"
#include <esp_log.h>
#include <esp_app_desc.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <cstring>
#include <esp_pthread.h>

#include "application.h"
#include "display.h"
#include "oled_display.h"
#include "board.h"
#include "settings.h"
#include "lvgl_theme.h"
#include "lvgl_display.h"
#include "lvgl_image.h"
#include "system_info.h"
#include "remote_camera.h"
#include "esp_jpeg_dec.h"

#define TAG "MCP"

// Static RemoteCamera instance for use when no hardware camera is available
static RemoteCamera *g_remote_camera = nullptr;

McpServer::McpServer()
{
}

McpServer::~McpServer()
{
    for (auto tool : tools_)
    {
        delete tool;
    }
    tools_.clear();
}

static bool DecodeAndDisplayRemoteJpeg(const uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "Starting JPEG decode, size: %d bytes", (int)len);
    
    auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
    if (!display) {
        ESP_LOGE(TAG, "Display not available");
        return false;
    }

    jpeg_dec_handle_t dec = nullptr;
    jpeg_dec_config_t cfg = {
        .output_type = JPEG_PIXEL_FORMAT_RGB565_LE,
        .rotate = JPEG_ROTATE_0D
    };
    
    if (jpeg_dec_open(&cfg, &dec) != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "Failed to open JPEG decoder");
        return false;
    }

    jpeg_dec_io_t io = {};
    jpeg_dec_header_info_t hdr = {};
    io.inbuf = (unsigned char*)data;
    io.inbuf_len = len;
    
    int ret = jpeg_dec_parse_header(dec, &io, &hdr);
    if (ret < 0) {
        ESP_LOGE(TAG, "Failed to parse JPEG header: %d", ret);
        jpeg_dec_close(dec);
        return false;
    }

    ESP_LOGI(TAG, "JPEG size: %dx%d", hdr.width, hdr.height);

    size_t sz = hdr.width * hdr.height * 2;
    uint8_t* buf = (uint8_t*)jpeg_calloc_align(sz, 16);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for decoded image", (int)sz);
        jpeg_dec_close(dec);
        return false;
    }

    io.outbuf = buf;
    int consumed = io.inbuf_len - io.inbuf_remain;
    io.inbuf = (unsigned char*)data + consumed;
    io.inbuf_len = io.inbuf_remain;
    
    ret = jpeg_dec_process(dec, &io);
    jpeg_dec_close(dec);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "JPEG decode failed: %d", ret);
        heap_caps_free(buf);
        return false;
    }
    
    ESP_LOGI(TAG, "JPEG decoded successfully, creating image");
    
    auto image = std::make_unique<LvglAllocatedImage>(
        buf, sz, hdr.width, hdr.height, hdr.width * 2, LV_COLOR_FORMAT_RGB565);
    
    display->SetPreviewImage(std::move(image));
    
    char msg[64];
    snprintf(msg, sizeof(msg), "📷 %dx%d", hdr.width, hdr.height);
    display->ShowNotification(msg, 3000);
    
    ESP_LOGI(TAG, "Preview displayed successfully");
    return true;
}


void McpServer::AddCommonTools()
{
    // *Important* To speed up the response time, we add the common tools to the beginning of
    // the tools list to utilize the prompt cache.
    // **重要** 为了提升响应速度，我们把常用的工具放在前面，利用 prompt cache 的特性。

    // Backup the original tools list and restore it after adding the common tools.
    auto original_tools = std::move(tools_);
    auto &board = Board::GetInstance();

    // Do not add custom tools here.
    // Custom tools must be added in the board's InitializeTools function.

    AddTool("self.get_device_status",
            "Provides the real-time information of the device, including the current status of the audio speaker, screen, battery, network, etc.\n"
            "Use this tool for: \n"
            "1. Answering questions about current condition (e.g. what is the current volume of the audio speaker?)\n"
            "2. As the first step to control the device (e.g. turn up / down the volume of the audio speaker, etc.)",
            PropertyList(),
            [&board](const PropertyList &properties) -> ReturnValue
            {
                return board.GetDeviceStatusJson();
            });

    AddTool("self.audio_speaker.set_volume",
            "Set the volume of the audio speaker. If the current volume is unknown, you must call `self.get_device_status` tool first and then call this tool.",
            PropertyList({Property("volume", kPropertyTypeInteger, 0, 100)}),
            [&board](const PropertyList &properties) -> ReturnValue
            {
                auto codec = board.GetAudioCodec();
                codec->SetOutputVolume(properties["volume"].value<int>());
                return true;
            });

    auto backlight = board.GetBacklight();
    if (backlight)
    {
        AddTool("self.screen.set_brightness",
                "Set the brightness of the screen.",
                PropertyList({Property("brightness", kPropertyTypeInteger, 0, 100)}),
                [backlight](const PropertyList &properties) -> ReturnValue
                {
                    uint8_t brightness = static_cast<uint8_t>(properties["brightness"].value<int>());
                    backlight->SetBrightness(brightness, true);
                    return true;
                });
    }

#ifdef HAVE_LVGL
    auto display = board.GetDisplay();
    if (display && display->GetTheme() != nullptr)
    {
        AddTool("self.screen.set_theme",
                "Set the theme of the screen. The theme can be `light` or `dark`.",
                PropertyList({Property("theme", kPropertyTypeString)}),
                [display](const PropertyList &properties) -> ReturnValue
                {
                    auto theme_name = properties["theme"].value<std::string>();
                    auto &theme_manager = LvglThemeManager::GetInstance();
                    auto theme = theme_manager.GetTheme(theme_name);
                    if (theme != nullptr)
                    {
                        display->SetTheme(theme);
                        return true;
                    }
                    return false;
                });
    }

    auto camera = board.GetCamera();
    if (camera)
    {
        AddTool("self.camera.take_photo",
                "Take a photo and explain it. Use this tool after the user asks you to see something.\n"
                "Args:\n"
                "  `question`: The question that you want to ask about the photo.\n"
                "Return:\n"
                "  A JSON object that provides the photo information.",
                PropertyList({Property("question", kPropertyTypeString)}),
                [camera](const PropertyList &properties) -> ReturnValue
                {
                    // Lower the priority to do the camera capture
                    TaskPriorityReset priority_reset(1);

                    if (!camera->Capture())
                    {
                        throw std::runtime_error("Failed to capture photo");
                    }
                    auto question = properties["question"].value<std::string>();
                    return camera->Explain(question);
                });
    }
    else
    {
        // Fallback to remote camera when no hardware camera is available
        AddTool("self.camera.take_photo",
                "Take a photo from remote camera and explain it. Use this tool after the user asks you to see something.\n"
                "Args:\n"
                "  `question`: The question that you want to ask about the photo.\n"
                "Return:\n"
                "  A JSON object that provides the photo information.",
                PropertyList({Property("question", kPropertyTypeString)}),
                [this](const PropertyList &properties) -> ReturnValue
                {
                    // Lower the priority to do the camera capture
                    TaskPriorityReset priority_reset(1);

                    // Get or create RemoteCamera instance
                    if (!g_remote_camera)
                    {
                        g_remote_camera = new RemoteCamera();
                    }

                    if (!g_remote_camera->Capture())
                    {
                        throw std::runtime_error("Failed to capture photo from remote camera");
                    }
                    auto question = properties["question"].value<std::string>();
                    return g_remote_camera->Explain(question);
                });
    }
#endif

    // Remote camera MCP tools (available even when no local camera is present)
    AddTool("self.camera.remote.set",
            "Configure remote camera parameters.\nArgs:\n  `capture_url`: The URL to GET the camera image from.\n  `explain_url`: The URL to send image explain requests to.\n  `token`: Optional bearer token for explain server.",
            PropertyList({Property("capture_url", kPropertyTypeString),
                          Property("explain_url", kPropertyTypeString),
                          Property("token", kPropertyTypeString)}),
            [this](const PropertyList &properties) -> ReturnValue
            {
                Settings settings("camera_remote", true);
                // The PropertyList passed here will contain the values set by DoToolCall
                settings.SetString("capture_url", properties["capture_url"].value<std::string>());
                settings.SetString("explain_url", properties["explain_url"].value<std::string>());
                settings.SetString("explain_token", properties["token"].value<std::string>());
                return true;
            });

    AddTool("self.camera.remote.get",
            "Get remote camera configuration.",
            PropertyList(),
            [this](const PropertyList &properties) -> ReturnValue
            {
                Settings settings("camera_remote", false);
                std::string capture_url = settings.GetString("capture_url", "");
                std::string explain_url = settings.GetString("explain_url", "");
                std::string explain_token = settings.GetString("explain_token", "");
                cJSON *json = cJSON_CreateObject();
                cJSON_AddStringToObject(json, "capture_url", capture_url.c_str());
                cJSON_AddStringToObject(json, "explain_url", explain_url.c_str());
                cJSON_AddStringToObject(json, "explain_token", explain_token.c_str());
                return json;
            });

    AddTool("self.camera.take_photo_remote",
            "Take a photo from configured remote camera and explain it. Args:\n  `question`: question for the AI analysis.",
            PropertyList({Property("question", kPropertyTypeString)}),
            [this](const PropertyList &properties) -> ReturnValue
            {
                TaskPriorityReset priority_reset(1);

                // Get or create RemoteCamera instance
                if (!g_remote_camera)
                {
                    g_remote_camera = new RemoteCamera();
                }

                // Capture from remote camera
                if (!g_remote_camera->Capture())
                {
                    throw std::runtime_error("Failed to capture from remote camera");
                }

                // Use the camera's Explain function to send to online AI model
                auto question = properties["question"].value<std::string>();
                return g_remote_camera->Explain(question);
            });

AddTool("self.camera.remote.preview",
    "Download and display remote camera image on screen.",
    PropertyList(),
    [this](const PropertyList& properties) -> ReturnValue {
        ESP_LOGI(TAG, "Remote camera preview requested");
        
        if (!g_remote_camera) {
            g_remote_camera = new RemoteCamera();
        }
        
        ESP_LOGI(TAG, "Starting capture...");
        if (!g_remote_camera->Capture()) {
            throw std::runtime_error("Failed to capture from remote camera");
        }

        size_t len = g_remote_camera->GetFrameLen();
        const uint8_t* data = g_remote_camera->GetFrameData();
        
        ESP_LOGI(TAG, "Captured %d bytes", (int)len);
        
        if (!data || len < 4) {
            throw std::runtime_error("No image data available");
        }
        
        if (data[0] != 0xFF || data[1] != 0xD8) {
            throw std::runtime_error("Invalid JPEG format");
        }
        
        ESP_LOGI(TAG, "Starting decode and display...");
        if (!DecodeAndDisplayRemoteJpeg(data, len)) {
            throw std::runtime_error("Failed to decode/display image");
        }
        
        ESP_LOGI(TAG, "Preview complete, returning response");
        
        cJSON* json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "size_bytes", (int)len);
        cJSON_AddStringToObject(json, "status", "displayed");
        return json;
    });

    // Restore the original tools list to the end of the tools list
    tools_.insert(tools_.end(), original_tools.begin(), original_tools.end());
}

void McpServer::AddUserOnlyTools()
{
    // System tools
    AddUserOnlyTool("self.get_system_info",
                    "Get the system information",
                    PropertyList(),
                    [this](const PropertyList &properties) -> ReturnValue
                    {
                        auto &board = Board::GetInstance();
                        return board.GetSystemInfoJson();
                    });

    AddUserOnlyTool("self.reboot", "Reboot the system",
                    PropertyList(),
                    [this](const PropertyList &properties) -> ReturnValue
                    {
                        auto &app = Application::GetInstance();
                        app.Schedule([&app]()
                                     {
                ESP_LOGW(TAG, "User requested reboot");
                vTaskDelay(pdMS_TO_TICKS(1000));

                app.Reboot(); });
                        return true;
                    });

    // Firmware upgrade
    AddUserOnlyTool("self.upgrade_firmware", "Upgrade firmware from a specific URL. This will download and install the firmware, then reboot the device.",
                    PropertyList({Property("url", kPropertyTypeString, "The URL of the firmware binary file to download and install")}),
                    [this](const PropertyList &properties) -> ReturnValue
                    {
                        auto url = properties["url"].value<std::string>();
                        ESP_LOGI(TAG, "User requested firmware upgrade from URL: %s", url.c_str());

                        auto &app = Application::GetInstance();
                        app.Schedule([url, &app]()
                                     {
                auto ota = std::make_unique<Ota>();
                
                bool success = app.UpgradeFirmware(*ota, url);
                if (!success) {
                    ESP_LOGE(TAG, "Firmware upgrade failed");
                } });

                        return true;
                    });

    // Display control
#ifdef HAVE_LVGL
    auto display = dynamic_cast<LvglDisplay *>(Board::GetInstance().GetDisplay());
    if (display)
    {
        AddUserOnlyTool("self.screen.get_info", "Information about the screen, including width, height, etc.",
                        PropertyList(),
                        [display](const PropertyList &properties) -> ReturnValue
                        {
                            cJSON *json = cJSON_CreateObject();
                            cJSON_AddNumberToObject(json, "width", display->width());
                            cJSON_AddNumberToObject(json, "height", display->height());
                            if (dynamic_cast<OledDisplay *>(display))
                            {
                                cJSON_AddBoolToObject(json, "monochrome", true);
                            }
                            else
                            {
                                cJSON_AddBoolToObject(json, "monochrome", false);
                            }
                            return json;
                        });

#if CONFIG_LV_USE_SNAPSHOT
        AddUserOnlyTool("self.screen.snapshot", "Snapshot the screen and upload it to a specific URL",
                        PropertyList({Property("url", kPropertyTypeString),
                                      Property("quality", kPropertyTypeInteger, 80, 1, 100)}),
                        [display](const PropertyList &properties) -> ReturnValue
                        {
                            auto url = properties["url"].value<std::string>();
                            auto quality = properties["quality"].value<int>();

                            std::string jpeg_data;
                            if (!display->SnapshotToJpeg(jpeg_data, quality))
                            {
                                throw std::runtime_error("Failed to snapshot screen");
                            }

                            ESP_LOGI(TAG, "Upload snapshot %u bytes to %s", jpeg_data.size(), url.c_str());

                            // 构造multipart/form-data请求体
                            std::string boundary = "----ESP32_SCREEN_SNAPSHOT_BOUNDARY";

                            auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);
                            http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
                            if (!http->Open("POST", url))
                            {
                                throw std::runtime_error("Failed to open URL: " + url);
                            }
                            {
                                // 文件字段头部
                                std::string file_header;
                                file_header += "--" + boundary + "\r\n";
                                file_header += "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.jpg\"\r\n";
                                file_header += "Content-Type: image/jpeg\r\n";
                                file_header += "\r\n";
                                http->Write(file_header.c_str(), file_header.size());
                            }

                            // JPEG数据
                            http->Write((const char *)jpeg_data.data(), jpeg_data.size());

                            {
                                // multipart尾部
                                std::string multipart_footer;
                                multipart_footer += "\r\n--" + boundary + "--\r\n";
                                http->Write(multipart_footer.c_str(), multipart_footer.size());
                            }
                            http->Write("", 0);

                            if (http->GetStatusCode() != 200)
                            {
                                throw std::runtime_error("Unexpected status code: " + std::to_string(http->GetStatusCode()));
                            }
                            std::string result = http->ReadAll();
                            http->Close();
                            ESP_LOGI(TAG, "Snapshot screen result: %s", result.c_str());
                            return true;
                        });

        AddUserOnlyTool("self.screen.preview_image", "Preview an image on the screen",
                        PropertyList({Property("url", kPropertyTypeString)}),
                        [display](const PropertyList &properties) -> ReturnValue
                        {
                            auto url = properties["url"].value<std::string>();
                            auto http = Board::GetInstance().GetNetwork()->CreateHttp(3);

                            if (!http->Open("GET", url))
                            {
                                throw std::runtime_error("Failed to open URL: " + url);
                            }
                            int status_code = http->GetStatusCode();
                            if (status_code != 200)
                            {
                                throw std::runtime_error("Unexpected status code: " + std::to_string(status_code));
                            }

                            size_t content_length = http->GetBodyLength();
                            char *data = (char *)heap_caps_malloc(content_length, MALLOC_CAP_8BIT);
                            if (data == nullptr)
                            {
                                throw std::runtime_error("Failed to allocate memory for image: " + url);
                            }
                            size_t total_read = 0;
                            while (total_read < content_length)
                            {
                                int ret = http->Read(data + total_read, content_length - total_read);
                                if (ret < 0)
                                {
                                    heap_caps_free(data);
                                    throw std::runtime_error("Failed to download image: " + url);
                                }
                                if (ret == 0)
                                {
                                    break;
                                }
                                total_read += ret;
                            }
                            http->Close();

                            auto image = std::make_unique<LvglAllocatedImage>(data, content_length);
                            display->SetPreviewImage(std::move(image));
                            return true;
                        });
#endif // CONFIG_LV_USE_SNAPSHOT
    }
#endif // HAVE_LVGL

    // Assets download url
    auto &assets = Assets::GetInstance();
    if (assets.partition_valid())
    {
        AddUserOnlyTool("self.assets.set_download_url", "Set the download url for the assets",
                        PropertyList({Property("url", kPropertyTypeString)}),
                        [](const PropertyList &properties) -> ReturnValue
                        {
                            auto url = properties["url"].value<std::string>();
                            Settings settings("assets", true);
                            settings.SetString("download_url", url);
                            return true;
                        });
    }
}

void McpServer::AddTool(McpTool *tool)
{
    // Prevent adding duplicate tools
    if (std::find_if(tools_.begin(), tools_.end(), [tool](const McpTool *t)
                     { return t->name() == tool->name(); }) != tools_.end())
    {
        ESP_LOGW(TAG, "Tool %s already added", tool->name().c_str());
        return;
    }

    ESP_LOGI(TAG, "Add tool: %s%s", tool->name().c_str(), tool->user_only() ? " [user]" : "");
    tools_.push_back(tool);
}

void McpServer::AddTool(const std::string &name, const std::string &description, const PropertyList &properties, std::function<ReturnValue(const PropertyList &)> callback)
{
    AddTool(new McpTool(name, description, properties, callback));
}

void McpServer::AddUserOnlyTool(const std::string &name, const std::string &description, const PropertyList &properties, std::function<ReturnValue(const PropertyList &)> callback)
{
    auto tool = new McpTool(name, description, properties, callback);
    tool->set_user_only(true);
    AddTool(tool);
}

void McpServer::ParseMessage(const std::string &message)
{
    cJSON *json = cJSON_Parse(message.c_str());
    if (json == nullptr)
    {
        ESP_LOGE(TAG, "Failed to parse MCP message: %s", message.c_str());
        return;
    }
    ParseMessage(json);
    cJSON_Delete(json);
}

void McpServer::ParseCapabilities(const cJSON *capabilities)
{
    auto vision = cJSON_GetObjectItem(capabilities, "vision");
    if (cJSON_IsObject(vision))
    {
        auto url = cJSON_GetObjectItem(vision, "url");
        auto token = cJSON_GetObjectItem(vision, "token");
        if (cJSON_IsString(url))
        {
            auto camera = Board::GetInstance().GetCamera();
            if (camera)
            {
                std::string url_str = std::string(url->valuestring);
                std::string token_str;
                if (cJSON_IsString(token))
                {
                    token_str = std::string(token->valuestring);
                }
                camera->SetExplainUrl(url_str, token_str);
            }
            else
            {
                // No local camera configured — persist explain URL/token for remote camera usage
                std::string url_str = std::string(url->valuestring);
                std::string token_str;
                if (cJSON_IsString(token))
                {
                    token_str = std::string(token->valuestring);
                }
                Settings settings("camera_remote", true);
                settings.SetString("explain_url", url_str);
                settings.SetString("explain_token", token_str);
            }
        }
    }
}

void McpServer::ParseMessage(const cJSON *json)
{
    // Check JSONRPC version
    auto version = cJSON_GetObjectItem(json, "jsonrpc");
    if (version == nullptr || !cJSON_IsString(version) || strcmp(version->valuestring, "2.0") != 0)
    {
        ESP_LOGE(TAG, "Invalid JSONRPC version: %s", version ? version->valuestring : "null");
        return;
    }

    // Check method
    auto method = cJSON_GetObjectItem(json, "method");
    if (method == nullptr || !cJSON_IsString(method))
    {
        ESP_LOGE(TAG, "Missing method");
        return;
    }

    auto method_str = std::string(method->valuestring);
    if (method_str.find("notifications") == 0)
    {
        return;
    }

    // Check params
    auto params = cJSON_GetObjectItem(json, "params");
    if (params != nullptr && !cJSON_IsObject(params))
    {
        ESP_LOGE(TAG, "Invalid params for method: %s", method_str.c_str());
        return;
    }

    auto id = cJSON_GetObjectItem(json, "id");
    if (id == nullptr || !cJSON_IsNumber(id))
    {
        ESP_LOGE(TAG, "Invalid id for method: %s", method_str.c_str());
        return;
    }
    auto id_int = id->valueint;

    if (method_str == "initialize")
    {
        if (cJSON_IsObject(params))
        {
            auto capabilities = cJSON_GetObjectItem(params, "capabilities");
            if (cJSON_IsObject(capabilities))
            {
                ParseCapabilities(capabilities);
            }
        }
        auto app_desc = esp_app_get_description();
        std::string message = "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" BOARD_NAME "\",\"version\":\"";
        message += app_desc->version;
        message += "\"}}";
        ReplyResult(id_int, message);
    }
    else if (method_str == "tools/list")
    {
        std::string cursor_str = "";
        bool list_user_only_tools = false;
        if (params != nullptr)
        {
            auto cursor = cJSON_GetObjectItem(params, "cursor");
            if (cJSON_IsString(cursor))
            {
                cursor_str = std::string(cursor->valuestring);
            }
            auto with_user_tools = cJSON_GetObjectItem(params, "withUserTools");
            if (cJSON_IsBool(with_user_tools))
            {
                list_user_only_tools = with_user_tools->valueint == 1;
            }
        }
        GetToolsList(id_int, cursor_str, list_user_only_tools);
    }
    else if (method_str == "tools/call")
    {
        if (!cJSON_IsObject(params))
        {
            ESP_LOGE(TAG, "tools/call: Missing params");
            ReplyError(id_int, "Missing params");
            return;
        }
        auto tool_name = cJSON_GetObjectItem(params, "name");
        if (!cJSON_IsString(tool_name))
        {
            ESP_LOGE(TAG, "tools/call: Missing name");
            ReplyError(id_int, "Missing name");
            return;
        }
        auto tool_arguments = cJSON_GetObjectItem(params, "arguments");
        if (tool_arguments != nullptr && !cJSON_IsObject(tool_arguments))
        {
            ESP_LOGE(TAG, "tools/call: Invalid arguments");
            ReplyError(id_int, "Invalid arguments");
            return;
        }
        DoToolCall(id_int, std::string(tool_name->valuestring), tool_arguments);
    }
    else
    {
        ESP_LOGE(TAG, "Method not implemented: %s", method_str.c_str());
        ReplyError(id_int, "Method not implemented: " + method_str);
    }
}

void McpServer::ReplyResult(int id, const std::string &result)
{
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id) + ",\"result\":";
    payload += result;
    payload += "}";
    Application::GetInstance().SendMcpMessage(payload);
}

void McpServer::ReplyError(int id, const std::string &message)
{
    std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
    payload += std::to_string(id);
    payload += ",\"error\":{\"message\":\"";
    payload += message;
    payload += "\"}}";
    Application::GetInstance().SendMcpMessage(payload);
}

void McpServer::GetToolsList(int id, const std::string &cursor, bool list_user_only_tools)
{
    const int max_payload_size = 8000;
    std::string json = "{\"tools\":[";

    bool found_cursor = cursor.empty();
    auto it = tools_.begin();
    std::string next_cursor = "";

    while (it != tools_.end())
    {
        // 如果我们还没有找到起始位置，继续搜索
        if (!found_cursor)
        {
            if ((*it)->name() == cursor)
            {
                found_cursor = true;
            }
            else
            {
                ++it;
                continue;
            }
        }

        if (!list_user_only_tools && (*it)->user_only())
        {
            ++it;
            continue;
        }

        // 添加tool前检查大小
        std::string tool_json = (*it)->to_json() + ",";
        if (json.length() + tool_json.length() + 30 > max_payload_size)
        {
            // 如果添加这个tool会超出大小限制，设置next_cursor并退出循环
            next_cursor = (*it)->name();
            break;
        }

        json += tool_json;
        ++it;
    }

    if (json.back() == ',')
    {
        json.pop_back();
    }

    if (json.back() == '[' && !tools_.empty())
    {
        // 如果没有添加任何tool，返回错误
        ESP_LOGE(TAG, "tools/list: Failed to add tool %s because of payload size limit", next_cursor.c_str());
        ReplyError(id, "Failed to add tool " + next_cursor + " because of payload size limit");
        return;
    }

    if (next_cursor.empty())
    {
        json += "]}";
    }
    else
    {
        json += "],\"nextCursor\":\"" + next_cursor + "\"}";
    }

    ReplyResult(id, json);
}

void McpServer::DoToolCall(int id, const std::string &tool_name, const cJSON *tool_arguments)
{
    auto tool_iter = std::find_if(tools_.begin(), tools_.end(),
                                  [&tool_name](const McpTool *tool)
                                  {
                                      return tool->name() == tool_name;
                                  });

    if (tool_iter == tools_.end())
    {
        ESP_LOGE(TAG, "tools/call: Unknown tool: %s", tool_name.c_str());
        ReplyError(id, "Unknown tool: " + tool_name);
        return;
    }

    PropertyList arguments = (*tool_iter)->properties();
    try
    {
        for (auto &argument : arguments)
        {
            bool found = false;
            if (cJSON_IsObject(tool_arguments))
            {
                auto value = cJSON_GetObjectItem(tool_arguments, argument.name().c_str());
                if (argument.type() == kPropertyTypeBoolean && cJSON_IsBool(value))
                {
                    argument.set_value<bool>(value->valueint == 1);
                    found = true;
                }
                else if (argument.type() == kPropertyTypeInteger && cJSON_IsNumber(value))
                {
                    argument.set_value<int>(value->valueint);
                    found = true;
                }
                else if (argument.type() == kPropertyTypeString && cJSON_IsString(value))
                {
                    argument.set_value<std::string>(value->valuestring);
                    found = true;
                }
            }

            if (!argument.has_default_value() && !found)
            {
                ESP_LOGE(TAG, "tools/call: Missing valid argument: %s", argument.name().c_str());
                ReplyError(id, "Missing valid argument: " + argument.name());
                return;
            }
        }
    }
    catch (const std::exception &e)
    {
        ESP_LOGE(TAG, "tools/call: %s", e.what());
        ReplyError(id, e.what());
        return;
    }

    // Use main thread to call the tool
    auto &app = Application::GetInstance();
    app.Schedule([this, id, tool_iter, arguments = std::move(arguments)]()
                 {
        try {
            ReplyResult(id, (*tool_iter)->Call(arguments));
        } catch (const std::exception& e) {
            ESP_LOGE(TAG, "tools/call: %s", e.what());
            ReplyError(id, e.what());
        } });
}

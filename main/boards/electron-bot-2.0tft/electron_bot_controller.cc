/*
    Electron Bot Robot Controller -MCP Protocol Version
*/

#include <cJSON.h>
#include <esp_log.h>

#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
#include "movements.h"
#include "sdkconfig.h"
#include "settings.h"
#include "freertos/task.h"

#define TAG "ElectronBotController"

struct ElectronBotActionParams {
    int action_type;
    int steps;
    int speed;
    int direction;
    int amount;
};

class ElectronBotController {
private:
    Otto electron_bot_;
    TaskHandle_t action_task_handle_ = nullptr;
    QueueHandle_t action_queue_;
    bool is_action_in_progress_ = false;

    enum ActionType {
        // hand movements 1-12
        ACTION_HAND_LEFT_UP = 1,      // 举左手
        ACTION_HAND_RIGHT_UP = 2,     // 举右手
        ACTION_HAND_BOTH_UP = 3,      // 举双手
        ACTION_HAND_LEFT_DOWN = 4,    // 放左手
        ACTION_HAND_RIGHT_DOWN = 5,   // 放右手
        ACTION_HAND_BOTH_DOWN = 6,    // 放双手
        ACTION_HAND_LEFT_WAVE = 7,    // 挥左手
        ACTION_HAND_RIGHT_WAVE = 8,   // 挥右手
        ACTION_HAND_BOTH_WAVE = 9,    // 挥双手
        ACTION_HAND_LEFT_FLAP = 10,   // 拍打左手
        ACTION_HAND_RIGHT_FLAP = 11,  // slap right hand
        ACTION_HAND_BOTH_FLAP = 12,   // clap hands

        // body movements 13-14
        ACTION_BODY_TURN_LEFT = 13,    // 左转
        ACTION_BODY_TURN_RIGHT = 14,   // 右转
        ACTION_BODY_TURN_CENTER = 15,  // 回中心

        // head movements 16-20
        ACTION_HEAD_UP = 16,          // look up
        ACTION_HEAD_DOWN = 17,        // bow your head
        ACTION_HEAD_NOD_ONCE = 18,    // nod once
        ACTION_HEAD_CENTER = 19,      // Return to center
        ACTION_HEAD_NOD_REPEAT = 20,  // nod continuously

        // System action 21
        ACTION_HOME = 21  // Reset to initial position
    };

    static void ActionTask(void* arg) {
        ElectronBotController* controller = static_cast<ElectronBotController*>(arg);
        ElectronBotActionParams params;
        controller->electron_bot_.AttachServos();

        while (true) {
            if (xQueueReceive(controller->action_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE) {
                ESP_LOGI(TAG, "perform action: %d", params.action_type);
                controller->is_action_in_progress_ = true;  // Start executing action

                // perform corresponding actions
                if (params.action_type >= ACTION_HAND_LEFT_UP &&
                    params.action_type <= ACTION_HAND_BOTH_FLAP) {
                    // 手部动作
                    controller->electron_bot_.HandAction(params.action_type, params.steps,
                                                         params.amount, params.speed);
                } else if (params.action_type >= ACTION_BODY_TURN_LEFT &&
                           params.action_type <= ACTION_BODY_TURN_CENTER) {
                    // 身体动作
                    int body_direction = params.action_type - ACTION_BODY_TURN_LEFT + 1;
                    controller->electron_bot_.BodyAction(body_direction, params.steps,
                                                         params.amount, params.speed);
                } else if (params.action_type >= ACTION_HEAD_UP &&
                           params.action_type <= ACTION_HEAD_NOD_REPEAT) {
                    // 头部动作
                    int head_action = params.action_type - ACTION_HEAD_UP + 1;
                    controller->electron_bot_.HeadAction(head_action, params.steps, params.amount,
                                                         params.speed);
                } else if (params.action_type == ACTION_HOME) {
                    // 复位动作
                    controller->electron_bot_.Home(true);
                }
                controller->is_action_in_progress_ = false;  // 动作执行完毕
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    void QueueAction(int action_type, int steps, int speed, int direction, int amount) {
        ESP_LOGI(TAG, "Motion control: Type=%d, Steps=%d, Speed=%d, Direction=%d, Amplitude=%d", action_type, steps,
                 speed, direction, amount);

        ElectronBotActionParams params = {action_type, steps, speed, direction, amount};
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void StartActionTaskIfNeeded() {
        if (action_task_handle_ == nullptr) {
            xTaskCreate(ActionTask, "electron_bot_action", 1024 * 4, this, configMAX_PRIORITIES - 1,
                        &action_task_handle_);
        }
    }

    void LoadTrimsFromNVS() {
        Settings settings("electron_trims", false);

        int right_pitch = settings.GetInt("right_pitch", 0);
        int right_roll = settings.GetInt("right_roll", 0);
        int left_pitch = settings.GetInt("left_pitch", 0);
        int left_roll = settings.GetInt("left_roll", 0);
        int body = settings.GetInt("body", 0);
        int head = settings.GetInt("head", 0);
        electron_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);
    }

public:
    ElectronBotController() {
        ESP_LOGI(TAG, "Initializing Servos..."); // <-- ADD THIS
        // electron_bot_.Init(Right_Pitch_Pin, Right_Roll_Pin, Left_Pitch_Pin, Left_Roll_Pin, Body_Pin,
        //                    Head_Pin);
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_LOGI(TAG, "Loading Trims from NVS..."); // <-- ADD THIS
        LoadTrimsFromNVS();
        
        ESP_LOGI(TAG, "Creating Action Queue..."); // <-- ADD THIS
        action_queue_ = xQueueCreate(10, sizeof(ElectronBotActionParams));

        ESP_LOGI(TAG, "Queueing Home Action..."); // <-- ADD THIS
        QueueAction(ACTION_HOME, 1, 1000, 0, 0);

        ESP_LOGI(TAG, "Registering MCP Tools..."); // <-- ADD THIS
        RegisterMcpTools();
        ESP_LOGI(TAG, "Electron Bot controller has been initialized and registered with mcp tool");
    }

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "Start registering Electron Bot MCP tool...");

        // 手部动作统一工具
        mcp_server.AddTool(
            "self.electron.hand_action",
            "Hand motion control. action: 1=raise hand, 2=let go, 3=wave, 4=slap; hand: 1=left hand, 2=right hand, 3=both hands;"
            "steps: number of action repetitions (1-10); speed: action speed (500-1500, the smaller the value, the faster); amount:"
            "Range of motion (10 50, only used for raising hands)",
            PropertyList({Property("action", kPropertyTypeInteger, 1, 1, 4),
                          Property("hand", kPropertyTypeInteger, 3, 1, 3),
                          Property("steps", kPropertyTypeInteger, 1, 1, 10),
                          Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                          Property("amount", kPropertyTypeInteger, 30, 10, 50)}),
            [this](const PropertyList& properties) -> ReturnValue {
                int action_type = properties["action"].value<int>();
                int hand_type = properties["hand"].value<int>();
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int amount = properties["amount"].value<int>();

                // Calculate specific movements based on movement type and hand type
                int base_action;
                switch (action_type) {
                    case 1:
                        base_action = ACTION_HAND_LEFT_UP;
                        break;  // 举手
                    case 2:
                        base_action = ACTION_HAND_LEFT_DOWN;
                        amount = 0;
                        break;  // 放手
                    case 3:
                        base_action = ACTION_HAND_LEFT_WAVE;
                        amount = 0;
                        break;  // 挥手
                    case 4:
                        base_action = ACTION_HAND_LEFT_FLAP;
                        amount = 0;
                        break;  // 拍打
                    default:
                        base_action = ACTION_HAND_LEFT_UP;
                }
                int action_id = base_action + (hand_type - 1);

                QueueAction(action_id, steps, speed, 0, amount);
                return true;
            });

        // 身体动作
        mcp_server.AddTool(
            "self.electron.body_turn",
            "Body turned. steps: number of turning steps (1-10); speed: turning speed (500-1500, the smaller the value, the faster); direction: "
            "Steering direction (1=turn left, 2=turn right, 3=return to center); angle: steering angle (0-90 degrees)",
            PropertyList({Property("steps", kPropertyTypeInteger, 1, 1, 10),
                          Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                          Property("direction", kPropertyTypeInteger, 1, 1, 3),
                          Property("angle", kPropertyTypeInteger, 45, 0, 90)}),
            [this](const PropertyList& properties) -> ReturnValue {
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int direction = properties["direction"].value<int>();
                int amount = properties["angle"].value<int>();

                int action;
                switch (direction) {
                    case 1:
                        action = ACTION_BODY_TURN_LEFT;
                        break;
                    case 2:
                        action = ACTION_BODY_TURN_RIGHT;
                        break;
                    case 3:
                        action = ACTION_BODY_TURN_CENTER;
                        break;
                    default:
                        action = ACTION_BODY_TURN_LEFT;
                }

                QueueAction(action, steps, speed, 0, amount);
                return true;
            });

        // 头部动作
        mcp_server.AddTool("self.electron.head_move",
                           "Head movement. action: 1=raise head, 2=lower head, 3=nod, 4=return to center, 5=continuously nod; steps: "
                           "Number of action repetitions (1-10); speed: action speed (500-1500, the smaller the value, the faster); angle: "
                           "Head rotation angle (1-15 degrees)",
                           PropertyList({Property("action", kPropertyTypeInteger, 3, 1, 5),
                                         Property("steps", kPropertyTypeInteger, 1, 1, 10),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("angle", kPropertyTypeInteger, 5, 1, 15)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int action_num = properties["action"].value<int>();
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int amount = properties["angle"].value<int>();
                               int action = ACTION_HEAD_UP + (action_num - 1);
                               QueueAction(action, steps, speed, 0, amount);
                               return true;
                           });

        // 系统工具
        mcp_server.AddTool("self.electron.stop", "立即停止", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               // Clear the queue but keep the task resident
                               xQueueReset(action_queue_);
                               is_action_in_progress_ = false;
                               QueueAction(ACTION_HOME, 1, 1000, 0, 0);
                               return true;
                           });

        mcp_server.AddTool("self.electron.get_status", "Get the robot status, return moving or idle",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               return is_action_in_progress_ ? "moving" : "idle";
                           });

        // Single servo calibration tool
        mcp_server.AddTool(
            "self.electron.set_trim",
            "Calibrate individual servo positions. Set the fine-tuning parameters of the specified servo to adjust the initial attitude of the electron bot. The settings will be permanently saved."
            "servo_type: Servo type (right_pitch: right arm rotation, right_roll: right arm push and pull, left_pitch: left arm rotation,"
            "left_roll:Left arm push and pull, body: body, head: head); "
            "trim_value: Fine adjustment value (30 to 30 degrees)",
            PropertyList({Property("servo_type", kPropertyTypeString, "right_pitch"),
                          Property("trim_value", kPropertyTypeInteger, 0, -30, 30)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string servo_type = properties["servo_type"].value<std::string>();
                int trim_value = properties["trim_value"].value<int>();

                ESP_LOGI(TAG, "Set servo trim: %s = %ddegree", servo_type.c_str(), trim_value);

                // Get all current trimming values
                Settings settings("electron_trims", true);
                int right_pitch = settings.GetInt("right_pitch", 0);
                int right_roll = settings.GetInt("right_roll", 0);
                int left_pitch = settings.GetInt("left_pitch", 0);
                int left_roll = settings.GetInt("left_roll", 0);
                int body = settings.GetInt("body", 0);
                int head = settings.GetInt("head", 0);

                // Update the trim value of the specified servo
                if (servo_type == "right_pitch") {
                    right_pitch = trim_value;
                    settings.SetInt("right_pitch", right_pitch);
                } else if (servo_type == "right_roll") {
                    right_roll = trim_value;
                    settings.SetInt("right_roll", right_roll);
                } else if (servo_type == "left_pitch") {
                    left_pitch = trim_value;
                    settings.SetInt("left_pitch", left_pitch);
                } else if (servo_type == "left_roll") {
                    left_roll = trim_value;
                    settings.SetInt("left_roll", left_roll);
                } else if (servo_type == "body") {
                    body = trim_value;
                    settings.SetInt("body", body);
                } else if (servo_type == "head") {
                    head = trim_value;
                    settings.SetInt("head", head);
                } else {
                    return "Error: Invalid servo type, please use: right_pitch, right_roll, left_pitch, "
                           "left_roll, body, head";
                }

                electron_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);

                QueueAction(ACTION_HOME, 1, 500, 0, 0);

                return "steering gear " + servo_type + " Fine-tuning is set to " + std::to_string(trim_value) +
                       "degree, permanently saved";
            });

        mcp_server.AddTool("self.electron.get_trims", "Get the current servo trim settings", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               Settings settings("electron_trims", false);

                               int right_pitch = settings.GetInt("right_pitch", 0);
                               int right_roll = settings.GetInt("right_roll", 0);
                               int left_pitch = settings.GetInt("left_pitch", 0);
                               int left_roll = settings.GetInt("left_roll", 0);
                               int body = settings.GetInt("body", 0);
                               int head = settings.GetInt("head", 0);

                               std::string result =
                                   "{\"right_pitch\":" + std::to_string(right_pitch) +
                                   ",\"right_roll\":" + std::to_string(right_roll) +
                                   ",\"left_pitch\":" + std::to_string(left_pitch) +
                                   ",\"left_roll\":" + std::to_string(left_roll) +
                                   ",\"body\":" + std::to_string(body) +
                                   ",\"head\":" + std::to_string(head) + "}";

                               ESP_LOGI(TAG, "Get fine-tuning settings: %s", result.c_str());
                               return result;
                           });

        mcp_server.AddTool("self.battery.get_level", "Get the robot battery power and charging status", PropertyList(),
                           [](const PropertyList& properties) -> ReturnValue {
                               auto& board = Board::GetInstance();
                               int level = 0;
                               bool charging = false;
                               bool discharging = false;
                               board.GetBatteryLevel(level, charging, discharging);

                               std::string status =
                                   "{\"level\":" + std::to_string(level) +
                                   ",\"charging\":" + (charging ? "true" : "false") + "}";
                               return status;
                           });

        ESP_LOGI(TAG, "Electron Bot Mcp tool registration completed");
    }

    ~ElectronBotController() {
        if (action_task_handle_ != nullptr) {
            vTaskDelete(action_task_handle_);
            action_task_handle_ = nullptr;
        }
        vQueueDelete(action_queue_);
    }
};

static ElectronBotController* g_electron_controller = nullptr;

void InitializeElectronBotController() {
    if (g_electron_controller == nullptr) {
        g_electron_controller = new ElectronBotController();
        ESP_LOGI(TAG, "Electron Bot controller has been initialized and registered with mcp tool");
    }
}
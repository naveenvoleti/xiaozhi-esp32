/*
    Electron Bot机器人控制器 - MCP协议版本 (Merged with Direct Motor Control)
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
#include "BuiltinLed.h"
#ifndef MotorSerial
#include "motion_control.h"
#endif
#include "led/circular_strip.h"

#define TAG "SparkyBotController"

struct SparkyBotActionParams
{
    int action_type;
    int steps;
    int speed;
    int direction;
    int amount;
};

class SparkyBotController
{
private:
    Otto sparky_bot_;
    TaskHandle_t action_task_handle_ = nullptr;
    QueueHandle_t action_queue_;
    bool is_action_in_progress_ = false;
    bool led_on_ = false;
    BuiltinLed &builtin_led = BuiltinLed::GetInstance();
#ifdef ENABLE_RGB_LED
    CircularStrip led{RGB_LED_GPIO, 1};
#endif
    // Watchdog for chassis safety
    int motion_watchdog_count_ = 0;
    TaskHandle_t watchdog_task_handle_ = nullptr;

    enum ActionType
    {
        // 手部动作 1-12
        ACTION_HAND_LEFT_UP = 1,     // 举左手
        ACTION_HAND_RIGHT_UP = 2,    // 举右手
        ACTION_HAND_BOTH_UP = 3,     // 举双手
        ACTION_HAND_LEFT_DOWN = 4,   // 放左手
        ACTION_HAND_RIGHT_DOWN = 5,  // 放右手
        ACTION_HAND_BOTH_DOWN = 6,   // 放双手
        ACTION_HAND_LEFT_WAVE = 7,   // 挥左手
        ACTION_HAND_RIGHT_WAVE = 8,  // 挥右手
        ACTION_HAND_BOTH_WAVE = 9,   // 挥双手
        ACTION_HAND_LEFT_FLAP = 10,  // 拍打左手
        ACTION_HAND_RIGHT_FLAP = 11, // 拍打右手
        ACTION_HAND_BOTH_FLAP = 12,  // 拍打双手

        // 身体动作 13-14
        ACTION_BODY_TURN_LEFT = 13,   // 左转
        ACTION_BODY_TURN_RIGHT = 14,  // 右转
        ACTION_BODY_TURN_CENTER = 15, // 回中心

        // 头部动作 16-20
        ACTION_HEAD_UP = 16,         // 抬头
        ACTION_HEAD_DOWN = 17,       // 低头
        ACTION_HEAD_NOD_ONCE = 18,   // 点头一次
        ACTION_HEAD_CENTER = 19,     // 回中心
        ACTION_HEAD_NOD_REPEAT = 20, // 连续点头

        // 系统动作 21
        ACTION_HOME = 21 // 复位到初始位置
    };

    static void ActionTask(void *arg)
    {
        SparkyBotController *controller = static_cast<SparkyBotController *>(arg);
        SparkyBotActionParams params;
        controller->sparky_bot_.AttachServos();

        while (true)
        {
            if (xQueueReceive(controller->action_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE)
            {
                ESP_LOGI(TAG, "Executing action: %d", params.action_type);
                controller->is_action_in_progress_ = true; // Start executing action

                // Execute corresponding action
                if (params.action_type >= ACTION_HAND_LEFT_UP &&
                    params.action_type <= ACTION_HAND_BOTH_FLAP)
                {
                    // Hand motion
                    controller->sparky_bot_.HandAction(params.action_type, params.steps,
                                                       params.amount, params.speed);
                }
                else if (params.action_type >= ACTION_BODY_TURN_LEFT &&
                         params.action_type <= ACTION_BODY_TURN_CENTER)
                {
                    // Body motion
                    int body_direction = params.action_type - ACTION_BODY_TURN_LEFT + 1;
                    controller->sparky_bot_.BodyAction(body_direction, params.steps,
                                                       params.amount, params.speed);
                }
                else if (params.action_type >= ACTION_HEAD_UP &&
                         params.action_type <= ACTION_HEAD_NOD_REPEAT)
                {
                    // Head movements
                    int head_action = params.action_type - ACTION_HEAD_UP + 1;
                    controller->sparky_bot_.HeadAction(head_action, params.steps, params.amount,
                                                       params.speed);
                }
                else if (params.action_type == ACTION_HOME)
                {
                    // Reset action
                    controller->sparky_bot_.Home(true);
                }
                controller->is_action_in_progress_ = false; // Action completed
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
#ifndef MotorSerial
    // Watchdog task to stop chassis if no commands received
    static void WatchdogTask(void *arg)
    {
        SparkyBotController *controller = static_cast<SparkyBotController *>(arg);

        while (true)
        {
            controller->motion_watchdog_count_++;

            // Stop chassis if no motion command for ~500ms (25 * 20ms)
            if (controller->motion_watchdog_count_ > 25)
            {
                controller->motion_watchdog_count_ = 0;
                spark_bot_motion_control(0.0f, 0.0f);
                ESP_LOGW(TAG, "Watchdog triggered - stopping chassis");
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
#endif

    void QueueAction(int action_type, int steps, int speed, int direction, int amount)
    {
        ESP_LOGI(TAG, "Motion control: Type=%d, Steps=%d, Speed=%d, Direction=%d, Amplitude=%d", action_type, steps,
                 speed, direction, amount);

        SparkyBotActionParams params = {action_type, steps, speed, direction, amount};
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void StartActionTaskIfNeeded()
    {
        if (action_task_handle_ == nullptr)
        {
            xTaskCreate(ActionTask, "electron_bot_action", 1024 * 4, this, configMAX_PRIORITIES - 1,
                        &action_task_handle_);
        }
    }
#ifndef MotorSerial
    void StartWatchdogTask()
    {
        if (watchdog_task_handle_ == nullptr)
        {
            xTaskCreate(WatchdogTask, "chassis_watchdog", 2048, this, 5, &watchdog_task_handle_);
            ESP_LOGI(TAG, "Chassis watchdog task started");
        }
    }
#endif
    void LoadTrimsFromNVS()
    {
        Settings settings("electron_trims", false);

        int right_pitch = settings.GetInt("right_pitch", 0);
        int right_roll = settings.GetInt("right_roll", 0);
        int left_pitch = settings.GetInt("left_pitch", 0);
        int left_roll = settings.GetInt("left_roll", 0);
        int body = settings.GetInt("body", 0);
        int head = settings.GetInt("head", 0);
        sparky_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);
    }
#ifdef MotorSerial
    void InitializeEchoUart()
    {
        uart_config_t uart_config = {
            .baud_rate = ECHO_UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        int intr_alloc_flags = 0;

        ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
        ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, UART_ECHO_TXD, UART_ECHO_RXD, UART_ECHO_RTS, UART_ECHO_CTS));

        SendUartMessage("w2");
    }
    void SendUartMessage(const char *command_str)
    {
        uint8_t len = strlen(command_str);
        uart_write_bytes(ECHO_UART_PORT_NUM, command_str, len);
        uart_write_bytes(ECHO_UART_PORT_NUM, "\n", 1);
        ESP_LOGI(TAG, "Sent command: %s", command_str);
    }
#else

    // Direct motor control (replaces UART communication)
    void ControlChassis(float x_value, float y_value)
    {
        motion_watchdog_count_ = 0; // Reset watchdog
        spark_bot_motion_control(x_value, y_value);
        ESP_LOGI(TAG, "Chassis control: x=%.2f, y=%.2f", x_value, y_value);
    }
#endif

public:
    SparkyBotController()
    {
// LoadTrimsFromNVS();
// action_queue_ = xQueueCreate(10, sizeof(SparkyBotActionParams));

// ESP_LOGI(TAG, "Motor control initialized");

// QueueAction(ACTION_HOME, 1, 1000, 0, 0);

// // Start watchdog for chassis safety
// StartWatchdogTask();
#ifndef MotorSerial
        // Initialize motor control directly
        // motor_ledc_init();
#else
        InitializeEchoUart();
#endif
        RegisterMcpTools();
        ESP_LOGI(TAG, "Electron Bot controller initialized and MCP tools registered");
    }

    void RegisterMcpTools()
    {
        auto &mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "Starting to register Electron Bot MCP tools...");

        // Hand motion control tool
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
            [this](const PropertyList &properties) -> ReturnValue
            {
                int action_type = properties["action"].value<int>();
                int hand_type = properties["hand"].value<int>();
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int amount = properties["amount"].value<int>();

                int base_action;
                switch (action_type)
                {
                case 1:
                    base_action = ACTION_HAND_LEFT_UP;
                    break;
                case 2:
                    base_action = ACTION_HAND_LEFT_DOWN;
                    amount = 0;
                    break;
                case 3:
                    base_action = ACTION_HAND_LEFT_WAVE;
                    amount = 0;
                    break;
                case 4:
                    base_action = ACTION_HAND_LEFT_FLAP;
                    amount = 0;
                    break;
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
            [this](const PropertyList &properties) -> ReturnValue
            {
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int direction = properties["direction"].value<int>();
                int amount = properties["angle"].value<int>();

                int action;
                switch (direction)
                {
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
                           "Head movement. action: 1=head up, 2=head down, 3=nod, 4=return to center, 5=continuous nod; steps: "
                           "Number of repetitions (1-10); speed: Movement speed (500-1500, smaller value is faster); angle: "
                           "Head rotation angle (1-15 degrees)",
                           PropertyList({Property("action", kPropertyTypeInteger, 3, 1, 5),
                                         Property("steps", kPropertyTypeInteger, 1, 1, 10),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("angle", kPropertyTypeInteger, 5, 1, 15)}),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               int action_num = properties["action"].value<int>();
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int amount = properties["angle"].value<int>();
                               int action = ACTION_HEAD_UP + (action_num - 1);
                               QueueAction(action, steps, speed, 0, amount);
                               return true;
                           });

        // 系统工具
        mcp_server.AddTool("self.electron.stop", "Stop immediately", PropertyList(),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               xQueueReset(action_queue_);
                               is_action_in_progress_ = false;
                               QueueAction(ACTION_HOME, 1, 1000, 0, 0);
                               return true;
                           });

        mcp_server.AddTool("self.electron.get_status", "Get the robot status, returns moving or idle",
                           PropertyList(), [this](const PropertyList &properties) -> ReturnValue
                           { return is_action_in_progress_ ? "moving" : "idle"; });

        // 单个舵机校准工具
        mcp_server.AddTool(
            "self.electron.set_trim",
            "Calibrate individual servo positions. Set the fine-tuning parameters of the specified servo to adjust the initial attitude of the electron bot. The settings will be permanently saved."
            "servo_type: Servo type (right_pitch: right arm rotation, right_roll: right arm push-pull, left_pitch: left arm rotation, "
            "left_roll: left arm push-pull, body: body, head: head); "
            "trim_value: Trim value (-30 to 30 degrees)",
            PropertyList({Property("servo_type", kPropertyTypeString, "right_pitch"),
                          Property("trim_value", kPropertyTypeInteger, 0, -30, 30)}),
            [this](const PropertyList &properties) -> ReturnValue
            {
                std::string servo_type = properties["servo_type"].value<std::string>();
                int trim_value = properties["trim_value"].value<int>();

                ESP_LOGI(TAG, "Setting servo trim: %s = %d degrees", servo_type.c_str(), trim_value);

                Settings settings("electron_trims", true);
                int right_pitch = settings.GetInt("right_pitch", 0);
                int right_roll = settings.GetInt("right_roll", 0);
                int left_pitch = settings.GetInt("left_pitch", 0);
                int left_roll = settings.GetInt("left_roll", 0);
                int body = settings.GetInt("body", 0);
                int head = settings.GetInt("head", 0);

                if (servo_type == "right_pitch")
                {
                    right_pitch = trim_value;
                    settings.SetInt("right_pitch", right_pitch);
                }
                else if (servo_type == "right_roll")
                {
                    right_roll = trim_value;
                    settings.SetInt("right_roll", right_roll);
                }
                else if (servo_type == "left_pitch")
                {
                    left_pitch = trim_value;
                    settings.SetInt("left_pitch", left_pitch);
                }
                else if (servo_type == "left_roll")
                {
                    left_roll = trim_value;
                    settings.SetInt("left_roll", left_roll);
                }
                else if (servo_type == "body")
                {
                    body = trim_value;
                    settings.SetInt("body", body);
                }
                else if (servo_type == "head")
                {
                    head = trim_value;
                    settings.SetInt("head", head);
                }
                else
                {
                    return "Error: Invalid servo type. Please use: right_pitch, right_roll, left_pitch, "
                           "left_roll, body, head";
                }

                sparky_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);
                QueueAction(ACTION_HOME, 1, 500, 0, 0);

                return "Servo " + servo_type + " trim set to " + std::to_string(trim_value) +
                       " degrees and saved permanently";
            });

        mcp_server.AddTool("self.electron.get_trims", "Get the current servo trim settings", PropertyList(),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
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

                               ESP_LOGI(TAG, "获取微调设置: %s", result.c_str());
                               return result;
                           });

        mcp_server.AddTool("self.battery.get_level", "Get the robot battery power and charging status", PropertyList(),
                           [](const PropertyList &properties) -> ReturnValue
                           {
                               auto &board = Board::GetInstance();
                               int level = 0;
                               bool charging = false;
                               bool discharging = false;
                               board.GetBatteryLevel(level, charging, discharging);

                               std::string status =
                                   "{\"level\":" + std::to_string(level) +
                                   ",\"charging\":" + (charging ? "true" : "false") + "}";
                               return status;
                           });

        // RGB LED Control
        mcp_server.AddTool("self.rgb.set_rgb",
                           "Set RGB LED color with individual red, green, and blue values (0-255)",
                           PropertyList({Property("r", kPropertyTypeInteger, 0, 0, 255),
                                         Property("g", kPropertyTypeInteger, 0, 0, 255),
                                         Property("b", kPropertyTypeInteger, 0, 0, 255)}),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               int r = properties["r"].value<int>();
                               int g = properties["g"].value<int>();
                               int b = properties["b"].value<int>();
                               led_on_ = true;
                               //    builtin_led.SetColor(r, g, b);
                               led.SetAllColor(StripColor(r, g, b));
                               led.SetBrightness(128, 64);
                               return true;
                           });

        mcp_server.AddTool("self.rgb.get_state",
                           "Get current RGB LED state including color values, blinking status, and on/off state",
                           PropertyList(),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               return builtin_led.GetState();
                           });

        mcp_server.AddTool("self.rgb.blink_rgb",
                           "Make RGB LED blink continuously at specified interval in milliseconds (100-5000ms)",
                           PropertyList({Property("interval", kPropertyTypeInteger, 500, 100, 5000)}),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               int interval = properties["interval"].value<int>();
                               //    builtin_led.Blink(BLINK_INFINITE, interval);
                               led.Blink(StripColor(255, 255, 255), interval);
                               led_on_ = true;
                               return true;
                           });

        mcp_server.AddTool("self.rgb.turn_off",
                           "Turn off the RGB LED completely",
                           PropertyList(),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               //    builtin_led.TurnOff();
                               led.TurnOff();
                               led_on_ = false;
                               return true;
                           });

        mcp_server.AddTool("self.rgb.set_brightness",
                           "Adjust RGB LED brightness level (0=off, 255=maximum)",
                           PropertyList({Property("brightness", kPropertyTypeInteger, 128, 0, 255)}),
                           [this](const PropertyList &properties) -> ReturnValue
                           {
                               int brightness = properties["brightness"].value<int>();
                               //    builtin_led.SetBrightness(static_cast<uint8_t>(brightness));
                               led.SetBrightness(static_cast<uint8_t>(brightness), static_cast<uint8_t>(brightness / 2), false);
                               return true;
                           });

// ===== CHASSIS CONTROL WITH DIRECT MOTOR CONTROL =====
#ifndef MotorSerial
        {
            mcp_server.AddTool("self.chassis.go_forward", "Move chassis forward", PropertyList(),
                               [this](const PropertyList &properties) -> ReturnValue
                               {
                                   ControlChassis(0.0f, 1.0f);
                                   return true;
                               });

            mcp_server.AddTool("self.chassis.go_back", "Move chassis backward", PropertyList(),
                               [this](const PropertyList &properties) -> ReturnValue
                               {
                                   ControlChassis(0.0f, -1.0f);
                                   return true;
                               });

            mcp_server.AddTool("self.chassis.turn_left", "Turn chassis left", PropertyList(),
                               [this](const PropertyList &properties) -> ReturnValue
                               {
                                   ControlChassis(-1.0f, 0.0f);
                                   return true;
                               });

            mcp_server.AddTool("self.chassis.turn_right", "Turn chassis right", PropertyList(),
                               [this](const PropertyList &properties) -> ReturnValue
                               {
                                   ControlChassis(1.0f, 0.0f);
                                   return true;
                               });

            mcp_server.AddTool("self.chassis.stop", "Stop chassis movement", PropertyList(),
                               [this](const PropertyList &properties) -> ReturnValue
                               {
                                   ControlChassis(0.0f, 0.0f);
                                   return true;
                               });
            mcp_server.AddTool(
                "self.chassis.move_custom",
                "Move chassis with custom x and y values (scaled by 100). x: turning (-100 to 100, negative=left, positive=right), "
                "y: forward/backward (-100 to 100, negative=backward, positive=forward)",
                PropertyList({Property("x", kPropertyTypeInteger, 0, -100, 100),
                              Property("y", kPropertyTypeInteger, 0, -100, 100)}),
                [this](const PropertyList &properties) -> ReturnValue
                {
                    int x = properties["x"].value<int>();
                    int y = properties["y"].value<int>();
                    // Convert from -100 to 100 range to -1.0 to 1.0 range
                    float x_float = x / 100.0f;
                    float y_float = y / 100.0f;
                    ControlChassis(x_float, y_float);
                    return true;
                });
            mcp_server.AddTool(
                "self.chassis.set_speed_coefficient",
                "Set motor speed coefficient. Use positive value to reduce right motor, negative to reduce left motor",
                PropertyList({Property("coefficient", kPropertyTypeInteger, 1, -10, 10)}),
                [this](const PropertyList &properties) -> ReturnValue
                {
                    int coefficient = properties["coefficient"].value<int>();
                    float coeff_float = coefficient / 10.0f;
                    set_motor_speed_coefficients(coeff_float);
                    ESP_LOGI(TAG, "Motor speed coefficient set to %.2f", coeff_float);
                    return true;
                });
        }
#else
// ===== CHASSIS CONTROL WITH UART COMMANDS (WITH TIMED SUPPORT) =====

// Go Forward with optional duration
mcp_server.AddTool("self.chassis.go_forward",
                   "Move forward. Optional: specify duration in milliseconds",
                   PropertyList({Property("duration_ms", kPropertyTypeInteger, 0, 0, 60000)}),
                   [this](const PropertyList &properties) -> ReturnValue {
                       int duration = properties["duration_ms"].value<int>();
                       
                       char cmd[64];
                       snprintf(cmd, sizeof(cmd), "x0.0y1.0t%d\n", duration);
                       SendUartMessage(cmd);
                       return true;
                   });

// Go Back with optional duration
mcp_server.AddTool("self.chassis.go_back",
                   "Move backward. Optional: specify duration in milliseconds",
                   PropertyList({Property("duration_ms", kPropertyTypeInteger, 0, 0, 60000)}),
                   [this](const PropertyList &properties) -> ReturnValue {
                       int duration = properties["duration_ms"].value<int>();
                       
                       char cmd[64];
                       snprintf(cmd, sizeof(cmd), "x0.0y-1.0t%d\n", duration);
                       SendUartMessage(cmd);
                       return true;
                   });

// Turn Left with optional duration
mcp_server.AddTool("self.chassis.turn_left",
                   "Turn left. Optional: specify duration in milliseconds",
                   PropertyList({Property("duration_ms", kPropertyTypeInteger, 0, 0, 60000)}),
                   [this](const PropertyList &properties) -> ReturnValue {
                       int duration = properties["duration_ms"].value<int>();
                       
                       char cmd[64];
                       snprintf(cmd, sizeof(cmd), "x1.0y0.0t%d\n", duration);
                       SendUartMessage(cmd);
                       return true;
                   });

// Turn Right with optional duration
mcp_server.AddTool("self.chassis.turn_right",
                   "Turn right. Optional: specify duration in milliseconds",
                   PropertyList({Property("duration_ms", kPropertyTypeInteger, 0, 0, 60000)}),
                   [this](const PropertyList &properties) -> ReturnValue {
                       int duration = properties["duration_ms"].value<int>();
                       
                       char cmd[64];
                       snprintf(cmd, sizeof(cmd), "x-1.0y0.0t%d\n", duration);
                       SendUartMessage(cmd);
                       return true;
                   });

// Stop immediately
mcp_server.AddTool("self.chassis.stop",
                   "Stop all motors immediately",
                   PropertyList(),
                   [this](const PropertyList &properties) -> ReturnValue {
                       char cmd[64];
                       snprintf(cmd, sizeof(cmd), "x0.0y0.0\n");
                       SendUartMessage(cmd);
                       return true;
                   });

// Custom movement with X, Y, and optional duration
// mcp_server.AddTool("self.chassis.custom_move",
//                    "Custom movement: x=rotation (-1.0 to 1.0), y=forward/back (-1.0 to 1.0), duration in ms",
//                    PropertyList({
//                        Property("x", kPropertyTypeInteger, 0.0, -1.0, 1.0),
//                        Property("y", kPropertyTypeInteger, 0.0, -1.0, 1.0),
//                        Property("duration_ms", kPropertyTypeInteger, 0, 0, 60000)
//                    }),
//                    [this](const PropertyList &properties) -> ReturnValue {
//                        float x = properties["x"].value<float>();
//                        float y = properties["y"].value<float>();
//                        int duration = properties["duration_ms"].value<int>();
                       
//                        char cmd[64];
//                        snprintf(cmd, sizeof(cmd), "x%.1fy%.1ft%d\n", x, y, duration);
//                        SendUartMessage(cmd);
//                        return true;
//                    });
#endif
        ESP_LOGI(TAG, "Electron Bot MCP tools registration completed");
    }

    ~SparkyBotController()
    {
        if (action_task_handle_ != nullptr)
        {
            vTaskDelete(action_task_handle_);
            action_task_handle_ = nullptr;
        }
        if (watchdog_task_handle_ != nullptr)
        {
            vTaskDelete(watchdog_task_handle_);
            watchdog_task_handle_ = nullptr;
        }
        vQueueDelete(action_queue_);
    }
};

static SparkyBotController *g_sparky_controller = nullptr;

void InitializeSparkyController()
{
    if (g_sparky_controller == nullptr)
    {
        g_sparky_controller = new SparkyBotController();
        // CircularStrip &led_strip = new
        // led_strip.strip_gpio_num_ = RGB_LED_GPIO;
        // led_strip.max_leds_ = 8;
        // led_strip.SetBrightness(128);
        ESP_LOGI(TAG, "Electron Bot controller has been initialized with direct motor control");
    }
}
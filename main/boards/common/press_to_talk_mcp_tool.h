#ifndef PRESS_TO_TALK_MCP_TOOL_H
#define PRESS_TO_TALK_MCP_TOOL_H

#include "mcp_server.h"
#include "settings.h"

// Reusable button-to-talk mode mcp tool class
class PressToTalkMcpTool {
private:
    bool press_to_talk_enabled_;

public:
    PressToTalkMcpTool();
    
    // Initialize tool and register to mcp server
    void Initialize();
    
    // Get the current button-to-talk mode status
    bool IsPressToTalkEnabled() const;

private:
    // MCP工具的回调函数
    ReturnValue HandleSetPressToTalk(const PropertyList& properties);
    
    // 内部方法：设置press to talk状态并保存到设置
    void SetPressToTalkEnabled(bool enabled);
};

#endif // PRESS_TO_TALK_MCP_TOOL_H 
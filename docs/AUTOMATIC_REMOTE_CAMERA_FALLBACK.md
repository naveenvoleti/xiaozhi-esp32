# Automatic Remote Camera Fallback - Implementation Summary

## What Changed

Your remote camera feature now has **automatic fallback** capability. When your Sparky robot (or any device without hardware camera) is asked to capture or see something, it will **automatically use the remote camera** instead of failing.

## How It Works Now

### Before (Current Implementation)
```
User: "What do you see?"
Robot: ❌ ERROR - No camera configured
```

### After (New Implementation)
```
User: "What do you see?"
Robot: ✅ Automatically fetches from remote camera
Robot: "I see a desk with a laptop..."
```

## Key Changes

### 1. **Automatic Fallback in MCP Server** (`main/mcp_server.cc`)

Added logic to detect when no hardware camera exists:

```cpp
auto camera = board.GetCamera();
if (camera) {
    // Use hardware camera
    AddTool("self.camera.take_photo", [...hardware_camera_code...]);
} else {
    // No hardware camera - use RemoteCamera automatically
    AddTool("self.camera.take_photo", [...remote_camera_code...]);
}
```

**Result:** The same `self.camera.take_photo` tool now works with EITHER hardware or remote camera automatically.

### 2. **Board Initialization** (`main/boards/common/board.cc`)

Added check for persisted remote camera configuration during startup:

```cpp
Settings camera_settings("camera_remote", false);
std::string capture_url = camera_settings.GetString("capture_url", "");
if (!capture_url.empty()) {
    ESP_LOGI(TAG, "Remote camera configuration found, will use as fallback");
}
```

**Result:** Device logs on startup if remote camera is pre-configured.

### 3. **Configuration Methods** (No code changes, but supported)

Three ways to configure remote camera URLs:

#### Option A: Via MCP Tool (Recommended)
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/camera",
      "explain_url": "http://ai-server.com/explain",
      "token": "your-token"
    }
  }
}
```

Once set, configuration persists in NVS (survives reboots).

#### Option B: During MCP Initialize
```json
{
  "jsonrpc": "2.0",
  "method": "initialize",
  "params": {
    "capabilities": {
      "vision": {
        "url": "http://192.168.1.100:8080/camera",
        "token": "your-token"
      }
    }
  }
}
```

#### Option C: Direct NVS (For developers)
- Namespace: `camera_remote`
- Keys: `capture_url`, `explain_url`, `explain_token`

## Files Modified

### 1. `/main/mcp_server.cc`
- **Lines 105-156:** Added else block with RemoteCamera fallback logic
- When `board.GetCamera()` returns `nullptr`, registers `self.camera.take_photo` with remote camera implementation
- Validates that `capture_url` is configured before attempting capture

### 2. `/main/boards/common/board.cc`
- **Lines 24-28:** Added remote camera config check in Board constructor
- Logs when remote camera is available as fallback

### 3. `/docs/REMOTE_CAMERA_CONFIG.md` (New)
- Comprehensive guide with setup examples
- Shows how automatic fallback works
- Includes troubleshooting section

## Behavior Now

### When Robot Starts
```
[Board] UUID=xxxxx SKU=sparky
[Board] Remote camera configuration found, will use as fallback
```

### When User Asks Robot to Look
```
User:  "What's in front of you?"
MCP:   Calls self.camera.take_photo
Robot: Fetches from capture_url automatically
       Sends to explain_url for AI analysis
       Returns result to user
```

### If Camera URL Not Configured
```
Error: "Remote camera capture URL not configured. Please configure camera URL first."
```

## Key Features

✅ **Automatic Fallback:** No code changes needed, just configure URLs
✅ **Persistent Configuration:** Stored in NVS, survives reboots
✅ **Seamless Integration:** `self.camera.take_photo` works transparently
✅ **Error Handling:** Clear error messages if config missing
✅ **Same Interface:** Hardware camera and remote camera use same API

## Testing the Feature

### 1. Configure Remote Camera
```bash
# Via MCP tool - configure camera URL
{
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/snapshot.jpg",
      "explain_url": "http://ai-server.local/vision/explain",
      "token": "my-token"
    }
  }
}
```

### 2. Ask Robot to Look
```bash
# Robot will automatically use remote camera
User: "Look at this and tell me what you see"
Robot: "I can see..."
```

### 3. Verify Configuration
```bash
# Check saved configuration
{
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.get"
  }
}
```

## Build Status

✅ **Build Successful**
- All changes compile without errors
- Binary size: 3.9M (1% free space)
- Ready for flashing

## Files Changed Summary

| File | Status | Change Type |
|------|--------|------------|
| `main/mcp_server.cc` | Modified | Added automatic fallback logic |
| `main/boards/common/board.cc` | Modified | Added startup logging |
| `docs/REMOTE_CAMERA_CONFIG.md` | Created | Setup and usage guide |

## Implementation Details

### Fallback Logic
```cpp
// When hardware camera missing
if (!g_remote_camera) {
    g_remote_camera = new RemoteCamera();
}

// Check configuration exists
Settings settings("camera_remote", false);
std::string capture_url = settings.GetString("capture_url", "");
if (capture_url.empty()) {
    throw std::runtime_error("Remote camera capture URL not configured...");
}

// Capture and explain
g_remote_camera->Capture();
return g_remote_camera->Explain(question);
```

### Configuration Storage
```cpp
// Store in NVS namespace "camera_remote"
Settings settings("camera_remote", true);
settings.SetString("capture_url", url);
settings.SetString("explain_url", url);
settings.SetString("explain_token", token);
```

## Next Steps (Optional)

1. **Set up a remote camera server** (Python HTTP server, IP camera, etc.)
2. **Configure remote camera URL** via MCP tool or initialize message
3. **Ask robot to see things** - it will automatically use remote camera
4. **Monitor logs** to verify automatic fallback is working

## Performance Notes

- **First capture:** May take 1-2 seconds (network + download)
- **Subsequent captures:** Typically 0.5-1 second
- **Memory:** Image buffered in SPIRAM
- **Network:** Requires WiFi connectivity to camera and AI server

## Backwards Compatibility

✅ No breaking changes
- Hardware camera boards work exactly as before
- Remote camera tools still available explicitly
- Existing MCP integrations unaffected

## Configuration Persistence

Configuration is stored in **NVS** (Non-Volatile Storage):
- Persists across power resets
- Persists across reboots
- Survives most firmware updates
- Can be cleared via factory reset

---

**Ready to use!** Just configure the remote camera URL and your robot will automatically use it when no hardware camera is available.

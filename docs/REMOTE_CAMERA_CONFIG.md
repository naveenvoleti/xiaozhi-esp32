# Remote Camera Configuration Guide

This guide explains how to automatically configure remote camera URLs for devices without hardware cameras (like Sparky robot).

## Quick Start

### Option 1: Configure via MCP Tools (Dynamic)

After the device starts, send these MCP requests to configure the remote camera:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/camera/snapshot",
      "explain_url": "http://your-ai-server.com/vision/explain",
      "token": "your-bearer-token"
    }
  }
}
```

**What happens next:**
- Configuration is stored in NVS (persists across reboots)
- When you ask the robot to capture or see something, it automatically uses the remote camera URL
- No need to specify the URL again

### Option 2: Pre-configure During MCP Initialization

During the MCP `initialize` handshake, include camera URLs in the capabilities:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "initialize",
  "params": {
    "protocolVersion": "2024-11-05",
    "capabilities": {
      "vision": {
        "url": "http://192.168.1.100:8080/camera/snapshot",
        "token": "your-bearer-token"
      }
    },
    "clientInfo": {
      "name": "MyClient",
      "version": "1.0.0"
    }
  }
}
```

**Note:** This sets the `explain_url` and `token`. You still need to set `capture_url` via `self.camera.remote.set` or it will be automatically set if the `url` field is used for capture.

### Option 3: Manual NVS Configuration (For Developers)

Store configuration directly in NVS using `nvs_set_str()` API:

- **Namespace:** `camera_remote`
- **Keys:**
  - `capture_url`: URL to GET the image from (required)
  - `explain_url`: URL to POST the image for AI analysis (optional)
  - `explain_token`: Bearer token for authentication (optional)

## How Automatic Fallback Works

1. **Device Startup:**
   - The board checks if there's a hardware camera
   - If not found (like Sparky), it prepares to use RemoteCamera as fallback

2. **User Command (e.g., "Look at this" or "What do you see?"):**
   - MCP server calls `self.camera.take_photo` tool
   - Tool checks if hardware camera exists
   - If not, it uses RemoteCamera instead
   - RemoteCamera fetches image from the configured `capture_url`
   - Image is sent to the `explain_url` for AI analysis
   - Result is returned to the user

3. **Configuration Check:**
   - If `capture_url` is not configured, you get an error asking you to configure it first
   - Once configured via any method, it persists across reboots

## Setup Examples

### Example 1: Simple Python HTTP Server

```python
from http.server import HTTPServer, SimpleHTTPRequestHandler
import os
import time

class ImageHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/camera/snapshot':
            # Serve a JPEG image file
            with open('camera.jpg', 'rb') as f:
                image_data = f.read()
            
            self.send_response(200)
            self.send_header('Content-Type', 'image/jpeg')
            self.send_header('Content-Length', len(image_data))
            self.end_headers()
            self.wfile.write(image_data)
        else:
            self.send_error(404)

if __name__ == '__main__':
    server = HTTPServer(('0.0.0.0', 8080), ImageHandler)
    print('Camera server running on http://0.0.0.0:8080/camera/snapshot')
    server.serve_forever()
```

Then configure the device:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/camera/snapshot",
      "explain_url": "http://your-ai-server.com/vision/explain",
      "token": "your-token"
    }
  }
}
```

### Example 2: Another Device as Camera

Device A (with hardware camera):
```cpp
// Returns JPEG image at /camera/capture endpoint
// Can be another ESP32 or IP camera
```

Device B (Sparky, without hardware camera):
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.50:8080/camera/capture",
      "explain_url": "http://ai-server.local/explain",
      "token": "device-token"
    }
  }
}
```

### Example 3: IP Camera (Wyze, Hikvision, etc.)

Many IP cameras provide HTTP endpoints for snapshots:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.200:8080/snapshot.jpg",
      "explain_url": "http://ai-server.local/explain",
      "token": "camera-token"
    }
  }
}
```

## Using the Remote Camera

Once configured, you can use the robot naturally:

### 1. Take a Photo and Analyze
```
User: "What do you see?"
Robot: [captures from remote camera, sends to AI server]
Robot: "I see a desk with a laptop..."
```

### 2. Show Live Preview on Screen
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.preview",
    "arguments": {}
  }
}
```

### 3. Check Configuration
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.get",
    "arguments": {}
  }
}
```

Returns:
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "capture_url": "http://192.168.1.100:8080/camera/snapshot",
    "explain_url": "http://your-ai-server.com/vision/explain",
    "explain_token": "your-bearer-token"
  }
}
```

## Automatic Behavior After Configuration

Once you've configured the remote camera:

1. **No more MCP tool calls needed** for basic operations
2. **Just ask the robot naturally:**
   - "Look at this"
   - "What do you see?"
   - "Describe what's in front of you"
3. Robot **automatically**:
   - Fetches image from configured `capture_url`
   - Sends to AI server at `explain_url`
   - Returns the analysis result

## Troubleshooting

### "Remote camera capture URL not configured"
- Run the `self.camera.remote.set` MCP tool first
- Or include camera config during `initialize` handshake
- Or set NVS values directly for your device

### "Failed to capture from remote camera"
- Check if the `capture_url` is accessible
- Try opening it in a browser: `http://your-capture-url`
- Verify network connectivity from ESP32 to camera server
- Check if the URL returns a valid JPEG image

### "Failed to upload photo, status code: 401"
- Bearer token might be missing or invalid
- Check the `explain_token` in config
- Regenerate the token on the AI server if needed

### "Remote camera captured but result is empty"
- The AI server at `explain_url` might be down
- Check if `explain_url` is correct
- Try a test request to the explain server directly
- Verify the server accepts multipart/form-data POST requests

## Configuration Persistence

- All settings are stored in ESP32's **NVS (Non-Volatile Storage)**
- Configuration persists across:
  - Power resets
  - Reboots
  - Firmware updates (usually)
- To reset: Clear NVS or factory reset the device

## Performance Considerations

- **Image Download Time:** Depends on network speed and image size
- **Memory:** Full image is buffered in SPIRAM (usually fine for <2MB images)
- **Latency:** Capture time + AI processing time
- **Optimization Tips:**
  - Compress images on the camera server
  - Reduce resolution for faster processing
  - Use fast network (WiFi 5/6)

## Architecture Diagram

```
┌─────────────────┐
│   Sparky Robot  │
│   (no camera)   │
└────────┬────────┘
         │
         │ asks to see something
         ▼
    ┌─────────┐
    │   MCP   │
    │ Server  │
    └────┬────┘
         │ calls self.camera.take_photo
         ▼
    ┌──────────────────┐
    │  RemoteCamera    │
    │  (fallback)      │
    └────┬──────┬──────┘
         │      │
    [1]  │      │  [2]
         ▼      ▼
    ┌──────────────┐    ┌────────────────┐
    │IP Camera/    │    │AI Vision       │
    │HTTP Server   │    │Server          │
    │(capture_url) │    │(explain_url)   │
    └──────────────┘    └────────────────┘
         │                    │
   [1] GET /snapshot      [2] POST multipart
         │                  image + question
         ▼                    │
      JPEG ←──────────────────┘
     Image                     │
         │                     │
         └─────────────────────┤
                               ▼
                    Analysis Result (JSON)
                               │
                               ▼
                       Back to User
```

## Summary

| Aspect | Before (no config) | After (with config) |
|--------|-------------------|---------------------|
| `self.camera.take_photo` | Not available | Automatically uses remote camera |
| Configuration | Via MCP tool every time | Persistent in NVS |
| Natural requests | Fail without hardware camera | Work automatically |
| Manual setup | Need to call MCP tools | One-time during initialization |

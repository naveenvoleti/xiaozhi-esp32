# Remote Camera Quick Reference

## The Problem (Before)
- Sparky robot has no hardware camera
- When you ask "What do you see?" → ERROR
- You had to manually call `self.camera.take_photo_remote` tool

## The Solution (After)
- Robot automatically uses remote camera when asked to look
- Same experience as hardware camera robots
- Just ask naturally: "Look at this" or "What do you see?"

---

## Quick Start (3 Steps)

### Step 1: Set up a Remote Camera Source
Choose one:

**Option A: Simple Python HTTP Server**
```python
from http.server import HTTPServer, BaseHTTPRequestHandler
import os

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/snap':
            with open('image.jpg', 'rb') as f:
                self.send_response(200)
                self.send_header('Content-Type', 'image/jpeg')
                self.end_headers()
                self.wfile.write(f.read())

HTTPServer(('0.0.0.0', 8080), Handler).serve_forever()
```
Start it: `python server.py`

**Option B: IP Camera**
- Wyze, Hikvision, etc. already have `/snapshot` endpoints

**Option C: Another ESP32 Device**
- Set up an HTTP endpoint returning JPEG

### Step 2: Configure Robot (One-time)
Send this MCP message once:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/snap",
      "explain_url": "http://your-ai-server.com/analyze",
      "token": "optional-bearer-token"
    }
  }
}
```

Configuration is now **persistent** - survives reboots!

### Step 3: Use Naturally
Just ask the robot:

```
You:   "What do you see?"
Robot: "I can see a person sitting at a desk with a laptop..."

You:   "Look at my face and describe it"
Robot: "You have a round face with brown eyes and a smile..."

You:   "Show me what's on the screen"
Robot: [displays image on its screen]
```

---

## How It Works

```
┌──────────────────┐
│  Ask Robot       │
│  "See something" │
└────────┬─────────┘
         │
         ▼
    ┌─────────────────────┐
    │ MCP self.camera.   │
    │ take_photo tool    │
    └────┬────────────────┘
         │
         ├─ Hardware camera available? 
         │  YES → Use it ✓
         │  NO  → Use RemoteCamera ↓
         │
         ▼
    ┌──────────────────┐
    │ Remote Camera    │
    │ (Automatic!)     │
    └────┬─────────────┘
         │
    [1]  ▼
    GET http://remote-camera/snap ──────┐
         │                              │
         │              ┌───────────────┘
         │              ▼
         │        ┌──────────────┐
         │        │ JPEG Image   │
         │        └──────────────┘
    [2]  │                │
         └────────┬───────┘
                  │
                  ▼
    POST http://ai-server/analyze
    [image + question + headers]
                  │
                  ▼
         ┌────────────────┐
         │ AI Result JSON │
         └────┬───────────┘
              │
              ▼
         Return to User
```

---

## Configuration Locations

### Where is my configuration stored?
- **NVS Namespace:** `camera_remote`
- **Device:** Persists in flash memory
- **Survives:** Power loss, reboots, most firmware updates

### Check Your Configuration
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.get"
  }
}
```

Response:
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "capture_url": "http://192.168.1.100:8080/snap",
    "explain_url": "http://your-ai-server.com/analyze",
    "explain_token": "your-token"
  }
}
```

---

## Tools Available

| Tool | Purpose | Need Config? |
|------|---------|--------------|
| `self.camera.take_photo` | Ask robot to look + analyze | capture_url + explain_url |
| `self.camera.remote.set` | Configure URLs (one-time) | - |
| `self.camera.remote.get` | Check current config | - |
| `self.camera.remote.preview` | Show live image on screen | capture_url |

---

## Troubleshooting

### Error: "Remote camera capture URL not configured"
**Solution:** Run `self.camera.remote.set` tool with your URLs

### Error: "Failed to capture from remote camera"
**Check:**
- Is your camera server running?
- Is the URL correct? Test in browser
- Is ESP32 connected to WiFi?
- Can you ping the camera from robot's network?

### Error: "Failed to upload photo, status code: 401"
**Solution:** Check or regenerate your bearer token in `explain_url` server

### Nothing happens when asking robot to "see"
**Check:**
- Is `capture_url` configured? (run `self.camera.remote.get`)
- Is it accessible? (try `curl http://your-url`)
- Any network errors in logs?

---

## Tips & Tricks

### Use a Static Image for Testing
```python
# Serve same image every time
def do_GET(self):
    self.send_response(200)
    self.send_header('Content-Type', 'image/jpeg')
    self.send_header('Content-Length', str(os.path.getsize('test.jpg')))
    self.end_headers()
    with open('test.jpg', 'rb') as f:
        self.wfile.write(f.read())
```

### Multiple Robots Can Share One Camera
```
Camera Server (1 device)
      ↓
Robot A ← uses same camera
Robot B ← uses same camera
Robot C ← uses same camera
```

### Chain Multiple Robots
Device with camera → Streams to HTTP → Sparky uses it

### Works with Different AI Servers
- OpenAI Vision API
- Google Cloud Vision
- Local LLMs
- Custom servers
- Any that accept multipart/form-data

---

## Common Setups

### Setup 1: Laptop Camera
```
Laptop with webcam
    ↓ (running HTTP server)
Remote Camera Server
    ↓ (Flask/Django/etc)
Sparky Robot
    ↓ (MCP connection)
Cloud AI API
```

### Setup 2: IP Camera Direct
```
Wyze/Hikvision Camera (http://camera.local/snapshot)
    ↓
Sparky Robot
    ↓
Cloud AI API
```

### Setup 3: Multi-Robot
```
ESP32 with Camera
    ↓ (HTTP endpoint)
Other ESP32 robots
    ↓
All ask same AI server
```

---

## Performance

| Operation | Time |
|-----------|------|
| First capture | 1-2 seconds |
| Subsequent | 0.5-1 second |
| Download 500KB image | ~1 second (WiFi) |
| AI analysis | Depends on server |
| **Total typical** | **3-5 seconds** |

---

## Remember

✅ **Set once** - Configuration persists
✅ **Works automatically** - No special MCP calls needed
✅ **Same as hardware** - Experience is identical to real camera
✅ **Just ask naturally** - Works with plain language

---

**Your robot is ready! Configure the remote camera and start asking it to see things!**

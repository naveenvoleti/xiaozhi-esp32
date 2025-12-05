# Remote Camera Setup Guide

This guide explains how to configure and use the remote camera feature in xiaozhi-esp32 when no hardware camera is connected.

## Overview

The remote camera feature allows the device to:
- Fetch images from a remote camera URL
- Display images on the device screen
- Send images to an AI explanation/vision server for analysis

This is useful when:
- No hardware camera is connected to the device
- You want to use a camera from another device (e.g., phone, IP camera, another ESP32)
- You need to test vision features without hardware

## Configuration Methods

### Method 1: MCP Tools (Dynamic Configuration)

Use the MCP protocol to configure remote camera at runtime. Send these JSONRPC 2.0 requests:

#### 1. Set Remote Camera Configuration

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.0.2.10:8080/capture.jpg",
      "explain_url": "https://ai.example.com/vision/analyze",
      "token": "YOUR_BEARER_TOKEN_HERE"
    }
  }
}
```

**Parameters:**
- `capture_url` (required): URL where the device GETs the image from. Should return a JPEG image file.
- `explain_url` (required): URL where the device POSTs the image for AI analysis. Should accept multipart/form-data with `file` field.
- `token` (optional): Bearer token for authentication with the explain server. Will be sent as `Authorization: Bearer <token>`.

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": true
}
```

#### 2. Verify Configuration

Retrieve the current remote camera configuration:

```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.get",
    "arguments": {}
  }
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "capture_url": "http://192.0.2.10:8080/capture.jpg",
    "explain_url": "https://ai.example.com/vision/analyze",
    "explain_token": "YOUR_BEARER_TOKEN_HERE"
  }
}
```

### Method 2: MCP Initialize Capabilities (One-time Setup)

During the MCP `initialize` handshake, pass the vision configuration:

```json
{
  "jsonrpc": "2.0",
  "id": 0,
  "method": "initialize",
  "params": {
    "capabilities": {
      "vision": {
        "url": "https://ai.example.com/vision/analyze",
        "token": "YOUR_BEARER_TOKEN_HERE"
      }
    }
  }
}
```

**Note:** This sets the `explain_url` and `token` in NVS storage. You still need to set `capture_url` separately using `self.camera.remote.set`.

### Method 3: Direct NVS Storage

Manually store values in NVS (ESP32's non-volatile storage):
- **Namespace:** `camera_remote`
- **Keys:**
  - `capture_url`: URL to GET image from
  - `explain_url`: URL to POST image to
  - `explain_token`: Bearer token (optional)

Use `nvs_set_str()` API if integrating with C/C++ code directly.

## Usage: Remote Camera MCP Tools

### 1. Preview Remote Camera on Device Screen

Display the current remote camera image on the device screen:

```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.preview",
    "arguments": {}
  }
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": true
}
```

**Use case:** Check what the remote camera sees before running analysis.

---

### 2. Capture and Analyze Remote Image

Fetch the image from the remote camera and send it to the AI explain server:

```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "method": "tools/call",
  "params": {
    "name": "self.camera.take_photo_remote",
    "arguments": {
      "question": "What objects are visible in this image?"
    }
  }
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "result": "{\"success\": true, \"analysis\": \"The image shows a desk with a laptop and coffee cup...\"}"
}
```

**Use case:** Have the AI analyze what the remote camera sees.

---

## Setup Examples

### Example 1: Using Python HTTP Server as Camera

Run a simple Python HTTP server that serves a JPEG image:

```python
from http.server import HTTPServer, SimpleHTTPRequestHandler
import os

class MyHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/capture.jpg':
            with open('test_image.jpg', 'rb') as f:
                self.send_response(200)
                self.send_header('Content-type', 'image/jpeg')
                self.end_headers()
                self.wfile.write(f.read())
        else:
            super().do_GET()

httpd = HTTPServer(('0.0.0.0', 8080), MyHandler)
print("Server running on http://0.0.0.0:8080")
httpd.serve_forever()
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
      "capture_url": "http://192.168.1.100:8080/capture.jpg",
      "explain_url": "https://your-vision-api.com/analyze",
      "token": "your-token"
    }
  }
}
```

---

### Example 2: Using Another ESP32 as Remote Camera

Device A (with hardware camera):
- Runs a simple HTTP server that streams camera frames
- Returns JPEG images via `/camera/capture.jpg` endpoint

Device B (without hardware camera):
- Configures capture_url pointing to Device A
- Can now use the remote camera feature

Setup on Device B:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.50:8080/camera/capture.jpg",
      "explain_url": "https://your-vision-api.com/analyze",
      "token": "your-token"
    }
  }
}
```

---

### Example 3: Using IP Camera

Many IP cameras provide HTTP endpoints to fetch snapshots. Example for Wyze Cam:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.150/api/videos/live?action=snapshots",
      "explain_url": "https://your-vision-api.com/analyze",
      "token": "your-token"
    }
  }
}
```

---

## Explain Server Requirements

The explain server (vision API endpoint) must:

1. **Accept POST requests** to the configured `explain_url`
2. **Accept multipart/form-data** with:
   - `file` field: JPEG image data
   - `question` field: Text question about the image
3. **Accept headers:**
   - `Device-Id`: MAC address of the device
   - `Client-Id`: UUID of the device
   - `Authorization: Bearer <token>` (if token is configured)
   - `Content-Type: multipart/form-data; boundary=...`
4. **Return HTTP 200** with JSON response body

**Example multipart payload sent by the device:**

```
--ESP32_CAMERA_BOUNDARY
Content-Disposition: form-data; name="question"

What's in front of the device?
--ESP32_CAMERA_BOUNDARY
Content-Disposition: form-data; name="file"; filename="camera.jpg"
Content-Type: image/jpeg

[binary JPEG data]
--ESP32_CAMERA_BOUNDARY--
```

**Example explain server response:**

```json
{
  "success": true,
  "result": "Analysis of the image...",
  "confidence": 0.95
}
```

---

## Data Storage

Remote camera configuration is stored in ESP32's NVS (Non-Volatile Storage):
- **Namespace:** `camera_remote`
- **Persists across reboots**
- **Max key/value size:** Limited by NVS (typically ~2KB per value)

To clear configuration:
- Reset the device
- Or use appropriate NVS management tools

---

## Troubleshooting

### "Remote camera capture_url is not configured"

**Problem:** The device doesn't have a capture_url configured.

**Solution:**
1. Call `self.camera.remote.set` with all parameters
2. Or check `self.camera.remote.get` to verify configuration was saved
3. Ensure the device has been rebooted or the setting was explicitly set

---

### "Failed to open capture URL"

**Problem:** The device cannot connect to the capture_url.

**Causes:**
- Network connectivity issue
- Wrong IP address or hostname
- Firewall blocking the connection
- Remote camera server is down

**Solution:**
1. Verify device has internet connectivity: `ping 8.8.8.8`
2. Check capture_url is correct: `curl http://192.0.2.10:8080/capture.jpg`
3. Verify firewall allows connection to remote camera

---

### "Unexpected status code: 404"

**Problem:** The capture_url returns a 404 (not found).

**Solution:**
1. Verify the capture_url is correct
2. Check the remote camera server is running
3. Ensure the image endpoint exists

---

### "Failed to connect to explain URL"

**Problem:** The device cannot reach the explain server.

**Causes:**
- Network issue
- Wrong explain_url
- Explain server is down
- DNS resolution failed

**Solution:**
1. Verify network connectivity
2. Check explain_url with curl
3. Ensure SSL certificates are valid (if using HTTPS)

---

### "Failed to upload photo, status code: 401"

**Problem:** The explain server rejected the request (authentication failed).

**Causes:**
- Missing or invalid token
- Token expired
- Wrong authentication method

**Solution:**
1. Verify token in `self.camera.remote.get`
2. Refresh/regenerate the token
3. Check explain server token format expectations

---

## Complete Setup Workflow

1. **Prepare a remote camera source:**
   - Set up an HTTP endpoint that returns JPEG images
   - Note the URL (e.g., `http://192.168.1.100:8080/capture.jpg`)

2. **Prepare an explain server:**
   - Set up a vision/AI API endpoint
   - Note the URL (e.g., `https://api.example.com/vision`)
   - Obtain authentication token if required

3. **Configure the device:**

   ```json
   {
     "jsonrpc": "2.0",
     "id": 1,
     "method": "tools/call",
     "params": {
       "name": "self.camera.remote.set",
       "arguments": {
         "capture_url": "http://192.168.1.100:8080/capture.jpg",
         "explain_url": "https://api.example.com/vision",
         "token": "your-auth-token"
       }
     }
   }
   ```

4. **Test preview:**

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

5. **Test analysis:**

   ```json
   {
     "jsonrpc": "2.0",
     "id": 3,
     "method": "tools/call",
     "params": {
       "name": "self.camera.take_photo_remote",
       "arguments": {
         "question": "What do you see?"
       }
     }
   }
   ```

---

## Performance Notes

- **Image size:** Entire remote image is downloaded into RAM before processing
- **Memory usage:** Depends on remote image size (typically 50KB-500KB for JPEG)
- **Network bandwidth:** One full image download per capture/preview operation
- **Latency:** Network latency + image download time + explain server processing time

For large images or slow networks, consider:
- Compressing images at the remote camera source
- Reducing image resolution
- Caching images if they don't change frequently

---

## Security Considerations

- **Store tokens securely:** Tokens are stored in plaintext in NVS. Use short-lived tokens.
- **Use HTTPS:** Always use HTTPS for explain_url when possible
- **Network isolation:** Only allow remote camera connections from trusted networks
- **Rate limiting:** Implement rate limiting on the explain server to prevent abuse

---

## Files Modified

- `main/mcp_server.cc`: Added remote camera MCP tools and ParseCapabilities handling

## Related Code

- Camera interface: `main/boards/common/camera.h`
- Settings storage: `main/settings.h`
- MCP server: `main/mcp_server.h`, `main/mcp_server.cc`

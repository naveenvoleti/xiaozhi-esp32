# Quick Diagnostic Guide

## The Most Common Cause

The remote camera capture fails because the **capture_url is not being persisted** to NVS.

### Verify This:

1. **Run this command:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.get",
    "arguments": {}
  }
}
```

2. **Check the response:**

If you see:
```json
{
  "capture_url": "",
  "explain_url": "",
  "explain_token": ""
}
```

**This is the problem!** The configuration is not being saved.

---

## How to Fix

### Solution 1: Verify Settings Storage is Working

The issue might be that the `self.camera.remote.set` tool isn't actually saving.

Check this by:
1. Set the URL
2. Immediately call `self.camera.remote.get`
3. If it's empty, the Settings class isn't saving properly

### Solution 2: Ensure Proper JSON in MCP Call

Make sure you're sending EXACT format:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/snapshot.jpg",
      "explain_url": "http://192.168.1.50:5000/analyze",
      "token": "my-token"
    }
  }
}
```

**Important:**
- All three fields MUST be present (capture_url, explain_url, token)
- Even if token is not needed, still include it (can be empty string)
- No typos in field names

### Solution 3: Check if Device Supports NVS Writing

The Settings class uses NVS. If NVS is full or corrupted, it won't save.

**Try this:**
1. Factory reset the device (clear all settings)
2. Try setting camera URL again
3. Check if it saves

### Solution 4: Manually Set via NVS (Advanced)

If the MCP tool isn't working, you can set directly. From another ESP32 app or C++ code:

```cpp
#include "settings.h"

Settings camera_settings("camera_remote", true);  // true = writable
camera_settings.SetString("capture_url", "http://192.168.1.100:8080/snapshot.jpg");
camera_settings.SetString("explain_url", "http://192.168.1.50:5000/analyze");
camera_settings.SetString("explain_token", "my-token");
```

---

## Debug Steps (In Order)

### Step 1: Verify Configuration Tool Response

```
Send: self.camera.remote.set with all parameters
Expected Result: {"result": true}
Actual Result: ?
```

If not true, configuration isn't being set!

### Step 2: Verify Configuration Persistence

```
Send: self.camera.remote.get 
Expected Result: URLs should match what you set
Actual Result: ?
```

If empty, NVS isn't saving!

### Step 3: Check Device Logs

When attempting capture, you should see:

```
[RemoteCamera] RemoteCamera initialized
  capture_url: http://192.168.1.100:8080/snapshot.jpg  ← Should NOT be empty!
  explain_url: http://192.168.1.50:5000/analyze         ← Should NOT be empty!
  explain_token: (set)
[RemoteCamera] Capturing from URL: http://192.168.1.100:8080/snapshot.jpg
```

If capture_url is empty here, the NVS read failed!

### Step 4: Test Network Connectivity

```bash
# From PC on same network:
curl -v http://192.168.1.100:8080/snapshot.jpg

# Should return:
# - HTTP 200
# - Some file size (Content-Length)
# - Binary JPEG data
```

If this fails, the camera server isn't accessible.

### Step 5: Test from Robot

The logs should show:

```
[RemoteCamera] Capture URL response status: 200
[RemoteCamera] Content length: XXXX bytes
[RemoteCamera] Remote camera capture successful: XXXX bytes
```

If you see different status (404, 401, 500), that's the actual issue!

---

## The Real Problem (Diagnosis)

| Symptom | Cause | Fix |
|---------|-------|-----|
| "Failed to capture" + empty config | NVS not saving | Reset device, try again |
| "Failed to capture" + config OK + logs empty | RemoteCamera not created | Check mcp_server.cc code |
| "Failed to capture" + logs show 404 | Wrong URL path | Fix URL in capture_url |
| "Failed to capture" + logs show 401 | Auth required | Add auth to URL |
| "Failed to capture" + logs show 500 | Server error | Fix camera server |
| "Failed to capture" + no logs | Network down | Check WiFi connection |

---

## Most Likely Fix

Based on your error, try this:

1. **Reboot the device**
2. **Set configuration again:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/snapshot.jpg",
      "explain_url": "http://192.168.1.50:5000/analyze",
      "token": ""
    }
  }
}
```

3. **Verify it saved:**
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

4. **Check the logs carefully for the exact error**

Share the logs output and I can tell you exactly what's wrong!

---

## What the Logs Should Show

**Success flow:**
```
[RemoteCamera] RemoteCamera initialized
[RemoteCamera]   capture_url: http://192.168.1.100:8080/snapshot.jpg
[RemoteCamera]   explain_url: http://192.168.1.50:5000/analyze
[RemoteCamera] Capturing from URL: http://192.168.1.100:8080/snapshot.jpg
[RemoteCamera] Capture URL response status: 200
[RemoteCamera] Content length: 50000 bytes
[RemoteCamera] Remote camera capture successful: 50000 bytes
```

**Failure scenarios:**

Failure 1: Config not loaded
```
[RemoteCamera] RemoteCamera initialized
[RemoteCamera]   capture_url: (not set)  ← PROBLEM!
[RemoteCamera]   explain_url: (not set)
[RemoteCamera]   explain_token: (not set)
```

Failure 2: Network error
```
[RemoteCamera] RemoteCamera initialized
[RemoteCamera]   capture_url: http://192.168.1.100:8080/snapshot.jpg
[RemoteCamera] Capturing from URL: http://192.168.1.100:8080/snapshot.jpg
[RemoteCamera] Failed to open capture URL: http://192.168.1.100:8080/snapshot.jpg ← PROBLEM!
```

Failure 3: Wrong status code
```
[RemoteCamera] Capture URL response status: 404  ← PROBLEM! (not 200)
```

---

## Next Actions

1. Check device logs when calling capture
2. Look for [RemoteCamera] messages
3. Identify which error message appears
4. Cross-reference with the fixes above

That will tell you exactly what to fix!

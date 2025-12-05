# Solving "Failed to capture from remote camera"

## TL;DR - Quick Fix

1. **Verify configuration is saved:**
```json
{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"self.camera.remote.get"}}
```

If you see empty URLs, the config isn't saving. Set it:
```json
{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"self.camera.remote.set","arguments":{"capture_url":"http://YOUR_IP:PORT/PATH","explain_url":"http://AI_SERVER:PORT/PATH","token":""}}}
```

2. **Check device logs** during capture attempt - look for [RemoteCamera] messages

3. **Identify the exact error** from the logs and use the troubleshooting guide below

---

## What's Happening

The error "Failed to capture from remote camera" means:
- RemoteCamera::Capture() returned false
- One of these failed:
  - Config loading
  - Network connection
  - HTTP request
  - Data reception

---

## Most Common Reasons & Fixes

### Reason 1: Configuration Not Saved (MOST COMMON)

**Signs:**
- Logs show `capture_url: (not set)`
- `self.camera.remote.get` returns empty values

**Fix:**
1. Set the configuration:
   - capture_url: `http://YOUR_CAMERA_IP:PORT/ENDPOINT`
   - explain_url: `http://YOUR_AI_SERVER:PORT/ENDPOINT`
   - token: can be empty string
2. Verify it saved with `self.camera.remote.get`
3. If still empty, reboot device and try again

### Reason 2: Wrong URL or Camera Server Not Running

**Signs:**
- Logs show status code `404` or `Failed to open`
- Can't reach camera from PC either

**Fix:**
1. Test URL from your PC:
   ```bash
   curl -v http://your-camera-url
   ```
2. Should get HTTP 200 and receive an image
3. Fix the URL if wrong
4. Start camera server if it's down

### Reason 3: Network Not Connected

**Signs:**
- Logs show `Network interface not available`
- WiFi icon not showing on robot

**Fix:**
1. Ensure robot is connected to WiFi
2. Check WiFi password
3. Restart WiFi connection

### Reason 4: Image Too Large

**Signs:**
- Logs show `Failed to allocate memory`

**Fix:**
1. Reduce image size on camera server
2. Use compression
3. Lower resolution

---

## Debugging Flowchart

```
Is capture failing?
│
├─ Check logs for [RemoteCamera] messages
│  │
│  ├─ No [RemoteCamera] logs?
│  │  └─ Issue in MCP tool, not remote camera
│  │
│  └─ Found [RemoteCamera] logs?
│     │
│     ├─ "capture_url: (not set)"?
│     │  └─ FIX: Set configuration with self.camera.remote.set
│     │
│     ├─ "Failed to open capture URL"?
│     │  └─ FIX: Check URL is correct, camera server running, network connected
│     │
│     ├─ "response status: 404"?
│     │  └─ FIX: Fix URL path on camera server
│     │
│     ├─ "response status: 401"?
│     │  └─ FIX: Add authentication to URL or use different endpoint
│     │
│     ├─ "No data read"?
│     │  └─ FIX: Check image file exists on server
│     │
│     └─ "Failed to allocate memory"?
│        └─ FIX: Reduce image size
```

---

## Complete Checklist

Before troubleshooting, verify:

- [ ] Robot is connected to WiFi (check status)
- [ ] Configuration saved (run `self.camera.remote.get`, should show URLs)
- [ ] Camera URL is correct format (http://IP:PORT/PATH)
- [ ] Camera server is running
- [ ] Can reach camera from PC: `curl http://your-url`
- [ ] Response is actually an image file (not error HTML)
- [ ] Explain server is running (if doing explain too)

---

## Quick Test Script

Try this Python test on your PC to verify the camera URL works:

```python
import requests
from PIL import Image
from io import BytesIO

url = "http://192.168.1.100:8080/snapshot.jpg"

try:
    response = requests.get(url, timeout=5)
    response.raise_for_status()
    
    # Check if it's actually an image
    img = Image.open(BytesIO(response.content))
    print(f"✓ Camera works! Image: {img.format} {img.size}")
    
    # Save to file
    img.save("test_capture.jpg")
    print(f"✓ Image saved to test_capture.jpg")
    
except Exception as e:
    print(f"✗ Error: {e}")
    print(f"✗ Camera at {url} is not working")
```

If this fails, your camera URL won't work on the robot either!

---

## Documentation Added

Check these files in `/docs/`:

1. **QUICK_DIAGNOSTIC.md** - Fast diagnosis
2. **DEBUG_CAPTURE_FAILURE.md** - Detailed debugging
3. **TROUBLESHOOT_CAPTURE_FAILURE.md** - Common issues & fixes

---

## Next Steps

1. **Open ESP32 console/logs**
2. **Run capture command**
3. **Look for [RemoteCamera] log messages**
4. **Match the error to the fixes above**
5. **Apply the fix**

If you're still stuck, share:
- The exact error from logs
- Output of `self.camera.remote.get`
- URL you're trying to use
- Result of curl test from PC

This will pinpoint the exact issue!

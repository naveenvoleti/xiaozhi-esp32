# Debugging: "Failed to capture from remote camera"

## What This Error Means

The `Capture()` function returned `false`. This can happen for several reasons:

1. ❌ Configuration not loaded from NVS
2. ❌ Network client creation failed
3. ❌ HTTP GET request failed
4. ❌ Received non-200 status code
5. ❌ No data received
6. ❌ Memory allocation failed

---

## Step-by-Step Debugging

### Step 1: Check if Configuration is Saved

Send this MCP request:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.get"
  }
}
```

**Expected response:**
```json
{
  "capture_url": "http://192.168.1.100:8080/snapshot.jpg",
  "explain_url": "http://192.168.1.50:5000/analyze",
  "explain_token": "your-token"
}
```

**If you get empty values:**
- First, set the configuration:
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
      "token": "your-token"
    }
  }
}
```

- Then immediately verify it saved:
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

**If it's still empty after setting:** NVS storage issue. Try rebooting device.

---

### Step 2: Check Device Logs During Capture

Watch the ESP32 console output while making a capture request.

**Expected log output:**
```
I (12345) RemoteCamera: RemoteCamera initialized
I (12346) RemoteCamera:   capture_url: http://192.168.1.100:8080/snapshot.jpg
I (12347) RemoteCamera:   explain_url: http://192.168.1.50:5000/analyze
I (12348) RemoteCamera:   explain_token: (set)
I (12349) RemoteCamera: Capturing from URL: http://192.168.1.100:8080/snapshot.jpg
I (12400) RemoteCamera: Capture URL response status: 200
I (12500) RemoteCamera: Content length: 50000 bytes
I (12900) RemoteCamera: Remote camera capture successful: 50000 bytes
```

---

### Step 3: Identify Where It Fails

Look for any of these error logs:

#### Error A: Configuration Not Found
```
I (12345) RemoteCamera: RemoteCamera initialized
I (12346) RemoteCamera:   capture_url: (not set)
I (12347) RemoteCamera:   explain_url: (not set)
```
**Problem:** NVS not saving configuration
**Fix:** Reboot, then set configuration again

#### Error B: Network Interface Not Available
```
E (12345) RemoteCamera: Network interface not available
```
**Problem:** WiFi not connected or Board network issue
**Fix:** Check WiFi connection, restart device

#### Error C: HTTP Client Creation Failed
```
E (12345) RemoteCamera: Failed to create HTTP client
```
**Problem:** Network resource exhausted
**Fix:** Restart device, or try reducing other network operations

#### Error D: Failed to Open URL
```
E (12345) RemoteCamera: Failed to open capture URL: http://192.168.1.100:8080/snapshot.jpg
```
**Problem:** Cannot reach camera server
**Fix:** 
- Check URL is correct
- Verify camera server is running
- Verify device is on same WiFi network
- Test with curl from PC: `curl http://192.168.1.100:8080/snapshot.jpg`

#### Error E: Wrong Status Code
```
I (12345) RemoteCamera: Capture URL response status: 404
E (12346) RemoteCamera: Unexpected status code from capture URL: 404
```
**Problem:** URL path doesn't exist on camera server
**Fix:** Check the exact endpoint path

Common status codes:
- `200` = Success ✓
- `404` = Not Found (wrong path)
- `401` = Unauthorized (needs auth)
- `403` = Forbidden (access denied)
- `500` = Server Error (camera app crashed)

#### Error F: No Data Received
```
E (12345) RemoteCamera: No data read from capture URL
```
**Problem:** Server returned empty response
**Fix:** Check camera server, ensure image file exists

#### Error G: Memory Allocation Failed
```
E (12345) RemoteCamera: Failed to allocate memory for remote image (size: 1000000)
```
**Problem:** Image too large, not enough SPIRAM
**Fix:** Reduce image size on camera server

---

## Common Fixes by Error

### "Remote camera capture URL not configured"
Configuration wasn't set. Solution:
```json
{
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://YOUR_CAMERA_IP:PORT/ENDPOINT",
      "explain_url": "http://YOUR_SERVER:PORT/ANALYZE",
      "token": "YOUR_TOKEN"
    }
  }
}
```

### "Failed to open capture URL"
URL not accessible. Solutions:
1. Test URL from PC: `curl http://your-url`
2. Check WiFi connection on robot
3. Verify camera server is running
4. Check firewall not blocking
5. Ensure correct IP and port

### "Unexpected status code: 404"
Wrong URL path. Solutions:
1. Check the exact path on camera server
2. Common patterns:
   - `http://192.168.1.100:8080/snapshot`
   - `http://192.168.1.100:8080/api/snapshot`
   - `http://192.168.1.100/snapshot.jpg`

### "Unexpected status code: 401"
Authentication required. Solutions:
1. Add auth to URL: `http://user:pass@server/snapshot`
2. Or use different endpoint that doesn't need auth

### "No data read from capture URL"
Empty response. Solutions:
1. Check image exists on server
2. Check server isn't returning error page (HTML) instead of image
3. Test: `curl -v http://your-url | file -`  should say "JPEG image"

---

## Test Procedure

1. **Clear and Reconfigure:**
   ```json
   {
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

2. **Verify Saved:**
   ```json
   {
     "method": "tools/call",
     "params": {
       "name": "self.camera.remote.get"
     }
   }
   ```
   Check all URLs appear in response

3. **Open Console/Serial Monitor**
   Monitor ESP32 output

4. **Attempt Capture:**
   ```json
   {
     "method": "tools/call",
     "params": {
       "name": "self.camera.take_photo",
       "arguments": {
         "question": "What do you see?"
       }
     }
   }
   ```

5. **Watch Logs:**
   Look for [RemoteCamera] messages
   Identify exact error point

---

## URL Format Requirements

Must be valid HTTP URL:
- ✅ `http://192.168.1.100:8080/snapshot.jpg`
- ✅ `http://camera.local:8080/image`
- ✅ `http://192.0.2.10:80/photo.jpg`
- ❌ `192.168.1.100:8080/snapshot.jpg` (missing http://)
- ❌ `http://localhost:8080/snap` (localhost won't work)
- ❌ `/snapshot.jpg` (relative path won't work)
- ❌ `file:///path/to/image.jpg` (file:// not supported)

---

## If All Else Fails

Collect this information:
1. Output of `self.camera.remote.get`
2. Full console logs from capture attempt
3. Result of `curl -v http://your-capture-url` from PC
4. Your network setup (WiFi name, IP addresses)

This will pinpoint the exact issue!

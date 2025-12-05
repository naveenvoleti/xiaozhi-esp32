# Troubleshooting: "Failed to capture from remote camera"

## Step 1: Verify Configuration is Saved

First, check that your URLs are actually saved in NVS:

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

Expected response:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "capture_url": "http://192.168.1.100:8080/snapshot.jpg",
    "explain_url": "http://ai-server.com/analyze",
    "explain_token": "your-token"
  }
}
```

**If URLs are empty or missing:**
- Make sure you called `self.camera.remote.set` with all parameters
- Check that the MCP request was successful (result: true)

---

## Step 2: Check Device Logs

When you try to capture, look for these log messages in ESP32 console:

```
[RemoteCamera] RemoteCamera initialized
  capture_url: http://192.168.1.100:8080/snapshot.jpg
  explain_url: (set)
  explain_token: (set)
[RemoteCamera] Capturing from URL: http://192.168.1.100:8080/snapshot.jpg
[RemoteCamera] Capture URL response status: 200
[RemoteCamera] Content length: 12345 bytes
[RemoteCamera] Remote camera capture successful: 12345 bytes
```

### If you see these errors instead:

**Error 1: "Remote camera capture URL not configured"**
- ✓ Solution: Run `self.camera.remote.set` with capture_url first
- Check with `self.camera.remote.get`

**Error 2: "Network interface not available"**
- ✓ Solution: Device not connected to WiFi
- Make sure device is connected and has internet

**Error 3: "Failed to create HTTP client"**
- ✓ Solution: Network resource issue
- Try again, restart device if persistent

**Error 4: "Failed to open capture URL"**
- ✓ Solution: Network path problem, check URL format
- Verify URL is correct: `http://IP:PORT/PATH`
- Make sure IP is on same network as robot

**Error 5: "Unexpected status code: 404"**
- ✓ Solution: URL path doesn't exist
- Check endpoint on your camera server
- Example: `http://192.168.1.100:8080/snapshot.jpg`

**Error 6: "Unexpected status code: 401"**
- ✓ Solution: Authentication required
- Add username/password to URL or use token
- Example: `http://user:pass@192.168.1.100:8080/snapshot.jpg`

**Error 7: "Unexpected status code: 500"**
- ✓ Solution: Remote server error
- Check if camera server is running
- Check server logs for errors

**Error 8: "No data read from capture URL"**
- ✓ Solution: Server returned empty response
- Check if image file exists
- Test URL with curl: `curl http://your-url/snapshot.jpg > test.jpg`

---

## Step 3: Test Manually

### Test with curl (From PC)

Verify your URL works:

```bash
# Test capture URL
curl -v http://192.168.1.100:8080/snapshot.jpg -o test.jpg
ls -lh test.jpg  # Should show file size > 0

# Test if file is valid JPEG
file test.jpg  # Should say "JPEG image data"
```

### Test from ESP32 Console

The robot will print detailed logs. Look for:
- URL being accessed
- HTTP status code
- Bytes downloaded
- Any errors

---

## Step 4: Common Issues & Fixes

### Issue: "Capture works but always captures same image"

This is normal! The capture_url returns whatever is at that endpoint.

**If this is a problem:**
- Use a live camera stream URL instead
- Or update the image on the server each time

### Issue: "Capture works, but explain fails (401 error)"

Authorization token issue.

**Fix:**
```json
{
  "method": "tools/call",
  "params": {
    "name": "self.camera.remote.set",
    "arguments": {
      "capture_url": "http://192.168.1.100:8080/snapshot.jpg",
      "explain_url": "http://ai-server.com/analyze",
      "token": "YOUR_NEW_TOKEN_HERE"
    }
  }
}
```

### Issue: "Image too large or takes too long"

Capture URL returns huge image (>5MB).

**Fix:**
- Compress image on camera server
- Reduce resolution
- Use smaller JPEG quality on camera

### Issue: "Can't reach camera server from robot"

Network connectivity problem.

**Fixes:**
1. Verify robot is on same WiFi network
2. Check firewall isn't blocking
3. Check camera server IP is correct (not localhost)
4. Ping test (if supported):
   - From PC: `ping 192.168.1.100` (robot IP)
   - Should get responses

---

## Step 5: Check Configuration Format

Your URLs must be in correct format:

**Valid:**
- `http://192.168.1.100:8080/snapshot.jpg`
- `http://camera.local:8080/image`
- `http://192.0.2.10:8080/capture.jpg`

**Invalid (will fail):**
- `192.168.1.100:8080/snapshot.jpg` (missing http://)
- `http://localhost:8080/snapshot.jpg` (localhost won't work on ESP32)
- `https://camera.local/snap` (HTTPS not fully supported)
- `/snapshot.jpg` (relative URL won't work)

---

## Step 6: Enable All Logs

To see more detailed debugging, try to enable verbose logging by checking console output while making the capture call:

1. Open serial monitor for ESP32
2. Run capture command
3. Watch for all [RemoteCamera] log messages
4. Share these logs to debug

---

## Complete Debug Checklist

- [ ] Configuration saved? Run `self.camera.remote.get`
- [ ] URLs are in correct format?
- [ ] Robot connected to WiFi?
- [ ] Camera server running and accessible?
- [ ] Can reach server from PC (curl test)?
- [ ] JPEG image is valid format?
- [ ] Image file size > 0?
- [ ] Console logs show capture attempt?
- [ ] Status codes are 200?
- [ ] Bytes are being downloaded?

---

## If Still Failing

Share these details:
1. Output of `self.camera.remote.get` 
2. Console logs from capture attempt (all [RemoteCamera] lines)
3. Your capture URL (for verification)
4. Result of: `curl -v http://your-capture-url`

This will help identify the exact issue!

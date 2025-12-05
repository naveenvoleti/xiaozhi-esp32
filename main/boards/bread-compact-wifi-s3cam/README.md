The hardware is based on the ESP32S3CAM development board, and the code is modified based on bread-compact-wifi-lcd
The camera used is OV2640
Note that because the camera occupies a lot of IO, it occupies two pins of USB 19 and 20 of ESP32S3.
For wiring methods, refer to the definition of pins in the config.h file.

 
# 编译配置命令

**配置编译目标为 ESP32S3：**

```bash
idf.py set-target esp32s3
```

**打开 menuconfig：**

```bash
idf.py menuconfig
```

**选择板子：**

```bash
Xiaozhi Assistant -> Board Type -> Breadboard new version wiring (WiFi) + LCD + Camera
```

**配置摄像头传感器：**

> **Note:**Confirm the camera sensor model and make sure the model is within the range supported by esp_cam_sensor. The current board uses OV2640, which is within the support range.

在 menuconfig 中按以下步骤启用对应型号的支持：

1. **Navigate to sensor configuration:**
   ```
   (Top) → Component config → Espressif Camera Sensors Configurations → Camera Sensor Configuration → Select and Set Camera Sensor
   ```

2. **选择传感器型号：**
   - 选中所需的传感器型号（OV2640）

3. **配置传感器参数：**
   - 按 → 进入传感器详细设置
   - 启用 **Auto detect**
   - 推荐将 **default output format** 调整为 **YUV422** 及合适的分辨率大小
   - （目前支持 YUV422、RGB565，YUV422 更节省内存空间）

**编译烧入：**

```bash
idf.py build flash
```
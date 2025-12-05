#include "remote_camera.h"
#include "board.h"
#include "system_info.h"
#include "settings.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <cstring>

#define TAG "RemoteCamera"

RemoteCamera::RemoteCamera()
    : frame_data_(nullptr), frame_len_(0), frame_width_(0), frame_height_(0) {
    // Load configuration from NVS
    Settings settings("camera_remote", false);
    capture_url_ = settings.GetString("capture_url", "");
    explain_url_ = settings.GetString("explain_url", "");
    explain_token_ = settings.GetString("explain_token", "");
    
    ESP_LOGI(TAG, "RemoteCamera initialized");
    ESP_LOGI(TAG, "  capture_url: %s", capture_url_.empty() ? "(not set)" : capture_url_.c_str());
    ESP_LOGI(TAG, "  explain_url: %s", explain_url_.empty() ? "(not set)" : explain_url_.c_str());
    ESP_LOGI(TAG, "  explain_token: %s", explain_token_.empty() ? "(not set)" : "(set)");
}

void RemoteCamera::SetExplainUrl(const std::string& url, const std::string& token) {
    explain_url_ = url;
    explain_token_ = token;
    // Persist to NVS
    Settings settings("camera_remote", true);
    settings.SetString("explain_url", url);
    settings.SetString("explain_token", token);
    ESP_LOGI(TAG, "Explain URL set to: %s", url.c_str());
}

void RemoteCamera::SetCaptureUrl(const std::string& url) {
    capture_url_ = url;
    // Persist to NVS
    Settings settings("camera_remote", true);
    settings.SetString("capture_url", url);
    ESP_LOGI(TAG, "Capture URL set to: %s", url.c_str());
}

bool RemoteCamera::Capture() {
    if (capture_url_.empty()) {
        ESP_LOGE(TAG, "Remote camera capture URL not configured");
        return false;
    }

    ESP_LOGI(TAG, "Capturing from URL: %s", capture_url_.c_str());

    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();
    if (!network) {
        ESP_LOGE(TAG, "Network interface not available");
        return false;
    }

    auto http = network->CreateHttp(3);
    if (!http) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    if (!http->Open("GET", capture_url_)) {
        ESP_LOGE(TAG, "Failed to open capture URL: %s", capture_url_.c_str());
        http->Close();
        return false;
    }

    int status_code = http->GetStatusCode();
    ESP_LOGI(TAG, "Capture URL response status: %d", status_code);
    
    if (status_code != 200) {
        ESP_LOGE(TAG, "Unexpected status code from capture URL: %d", status_code);
        http->Close();
        return false;
    }

    size_t content_length = http->GetBodyLength();
    ESP_LOGI(TAG, "Content length: %d bytes", (int)content_length);
    
    if (content_length == 0) {
        ESP_LOGE(TAG, "Remote server returned empty response");
        http->Close();
        return false;
    }

    // Allocate buffer for the image data
    uint8_t* data = (uint8_t*)heap_caps_malloc(content_length, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (data == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate memory for remote image (size: %d)", (int)content_length);
        http->Close();
        return false;
    }

    // Read all data from HTTP stream
    size_t total_read = 0;
    int retry_count = 0;
    const int max_retries = 5;
    
    while (total_read < content_length && retry_count < max_retries) {
        int ret = http->Read((char*)data + total_read, content_length - total_read);
        
        if (ret < 0) {
            ESP_LOGE(TAG, "HTTP Read error at offset %d, retry %d/%d", 
                     (int)total_read, retry_count + 1, max_retries);
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait before retry
            continue;
        }
        
        if (ret == 0) {
            ESP_LOGW(TAG, "HTTP stream ended at %d/%d bytes", 
                     (int)total_read, (int)content_length);
            break;
        }
        
        total_read += ret;
        retry_count = 0; // Reset retry count on successful read
    }
    
    http->Close();

    if (total_read == 0) {
        ESP_LOGE(TAG, "No data read from capture URL after %d retries", max_retries);
        heap_caps_free(data);
        return false;
    }

    if (total_read < content_length) {
        ESP_LOGW(TAG, "Incomplete read: got %d bytes, expected %d bytes", 
                 (int)total_read, (int)content_length);
    } else {
        ESP_LOGI(TAG, "Successfully downloaded %d bytes", (int)total_read);
    }

    // Validate JPEG format
    if (total_read < 2) {
        ESP_LOGE(TAG, "Downloaded data too small: %d bytes", (int)total_read);
        heap_caps_free(data);
        return false;
    }

    uint8_t* final_data = data;
    size_t final_len = total_read;

    ESP_LOGI(TAG, "Image header: 0x%02X 0x%02X, size: %d bytes", 
             data[0], data[1], (int)total_read);

    // Check if this is pure JPEG
    if (data[0] == 0xFF && data[1] == 0xD8) {
        // Verify JPEG end marker
        if (total_read >= 2 && data[total_read - 2] == 0xFF && data[total_read - 1] == 0xD9) {
            ESP_LOGI(TAG, "Valid JPEG with proper start and end markers");
        } else {
            ESP_LOGW(TAG, "JPEG start marker found but end marker missing or corrupt");
            ESP_LOGW(TAG, "Last 4 bytes: 0x%02X 0x%02X 0x%02X 0x%02X", 
                     data[total_read-4], data[total_read-3], 
                     data[total_read-2], data[total_read-1]);
        }
    } else {
        // Try to extract JPEG from MJPEG stream
        ESP_LOGW(TAG, "Not a pure JPEG (header: 0x%02X 0x%02X), attempting MJPEG extraction", 
                 data[0], data[1]);
        
        // Find JPEG start marker (0xFF 0xD8)
        uint8_t* jpeg_start = nullptr;
        for (size_t i = 0; i < total_read - 1; i++) {
            if (data[i] == 0xFF && data[i + 1] == 0xD8) {
                jpeg_start = data + i;
                ESP_LOGI(TAG, "Found JPEG start at offset %d", (int)i);
                break;
            }
        }

        if (!jpeg_start) {
            ESP_LOGE(TAG, "No JPEG start marker found in data");
            heap_caps_free(data);
            return false;
        }

        // Find JPEG end marker (0xFF 0xD9)
        uint8_t* jpeg_end = nullptr;
        for (size_t i = total_read - 1; i > (size_t)(jpeg_start - data + 1); i--) {
            if (data[i - 1] == 0xFF && data[i] == 0xD9) {
                jpeg_end = data + i + 1;
                ESP_LOGI(TAG, "Found JPEG end at offset %d", (int)i);
                break;
            }
        }

        if (!jpeg_end) {
            ESP_LOGE(TAG, "No JPEG end marker found in data");
            heap_caps_free(data);
            return false;
        }

        size_t jpeg_size = jpeg_end - jpeg_start;
        ESP_LOGI(TAG, "Extracted JPEG size: %d bytes", (int)jpeg_size);

        // Copy JPEG data to new buffer
        uint8_t* jpeg_data = (uint8_t*)heap_caps_malloc(jpeg_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!jpeg_data) {
            ESP_LOGE(TAG, "Failed to allocate memory for extracted JPEG");
            heap_caps_free(data);
            return false;
        }
        
        memcpy(jpeg_data, jpeg_start, jpeg_size);
        heap_caps_free(data);
        final_data = jpeg_data;
        final_len = jpeg_size;
    }

    // Free previous frame data
    if (frame_data_) {
        heap_caps_free(frame_data_);
    }

    frame_data_ = final_data;
    frame_len_ = final_len;
    frame_width_ = 320;
    frame_height_ = 240;

    ESP_LOGI(TAG, "Remote camera capture successful: %d bytes stored", (int)final_len);
    return true;
}

bool RemoteCamera::SetHMirror(bool enabled) {
    ESP_LOGW(TAG, "SetHMirror not supported for remote camera");
    return false;
}

bool RemoteCamera::SetVFlip(bool enabled) {
    ESP_LOGW(TAG, "SetVFlip not supported for remote camera");
    return false;
}

std::string RemoteCamera::Explain(const std::string& question) {
    if (explain_url_.empty()) {
        throw std::runtime_error("Explain URL not configured for remote camera");
    }

    if (!frame_data_ || frame_len_ == 0) {
        throw std::runtime_error("No frame data available. Call Capture() first.");
    }

    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();
    auto http = network->CreateHttp(3);

    std::string boundary = "----ESP32_CAMERA_BOUNDARY";

    // Configure HTTP headers
    http->SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());
    http->SetHeader("Client-Id", board.GetUuid().c_str());
    if (!explain_token_.empty()) {
        http->SetHeader("Authorization", "Bearer " + explain_token_);
    }
    http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    if (!http->Open("POST", explain_url_)) {
        ESP_LOGE(TAG, "Failed to connect to explain URL: %s", explain_url_.c_str());
        throw std::runtime_error("Failed to connect to explain URL");
    }

    // Send question field
    std::string question_field;
    question_field += "--" + boundary + "\r\n";
    question_field += "Content-Disposition: form-data; name=\"question\"\r\n";
    question_field += "\r\n";
    question_field += question + "\r\n";
    http->Write(question_field.c_str(), question_field.size());

    // Send file header
    std::string file_header;
    file_header += "--" + boundary + "\r\n";
    file_header += "Content-Disposition: form-data; name=\"file\"; filename=\"camera.jpg\"\r\n";
    file_header += "Content-Type: image/jpeg\r\n";
    file_header += "\r\n";
    http->Write(file_header.c_str(), file_header.size());

    // Send image data
    http->Write((const char*)frame_data_, frame_len_);

    // Send multipart footer
    std::string multipart_footer;
    multipart_footer += "\r\n--" + boundary + "--\r\n";
    http->Write(multipart_footer.c_str(), multipart_footer.size());
    http->Write("", 0);

    int status_code = http->GetStatusCode();
    if (status_code != 200) {
        ESP_LOGE(TAG, "Failed to upload photo to explain server, status code: %d", status_code);
        http->Close();
        throw std::runtime_error("Failed to upload photo, status code: " + std::to_string(status_code));
    }

    std::string result = http->ReadAll();
    http->Close();

    ESP_LOGI(TAG, "Explain successful, response length: %d bytes", (int)result.length());
    return result;
}
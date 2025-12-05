#ifndef REMOTE_CAMERA_H
#define REMOTE_CAMERA_H

#include "camera.h"
#include <string>
#include <cstdint>

/**
 * Remote Camera Adapter
 * 
 * Implements the Camera interface for fetching images from a remote HTTP endpoint.
 * Can be used as a fallback when no hardware camera is available.
 */
class RemoteCamera : public Camera {
public:
    RemoteCamera();
    ~RemoteCamera() = default;

    void SetExplainUrl(const std::string& url, const std::string& token) override;
    bool Capture() override;
    bool SetHMirror(bool enabled) override;
    bool SetVFlip(bool enabled) override;
    std::string Explain(const std::string& question) override;

    // Set the remote camera capture URL
    void SetCaptureUrl(const std::string& url);
    
    // Get the currently captured frame data
    const uint8_t* GetFrameData() const { return frame_data_; }
    size_t GetFrameLen() const { return frame_len_; }
    uint16_t GetFrameWidth() const { return frame_width_; }
    uint16_t GetFrameHeight() const { return frame_height_; }

private:
    std::string capture_url_;
    std::string explain_url_;
    std::string explain_token_;
    
    uint8_t* frame_data_;
    size_t frame_len_;
    uint16_t frame_width_;
    uint16_t frame_height_;
};

#endif // REMOTE_CAMERA_H

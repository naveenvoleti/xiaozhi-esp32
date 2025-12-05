#pragma once

#include "display/lcd_display.h"

/**
 * @brief Electron机器人GIF表情显示类
 * Inherit spi lcd display and add gif expression support through emoji collection
 */
class ElectronEmojiDisplay : public SpiLcdDisplay {
   public:
    /**
     * @brief Constructor, parameters are the same as spi lcd display
     */
    ElectronEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy);

    virtual ~ElectronEmojiDisplay() = default;
    virtual void SetStatus(const char* status) override;

   private:
    void InitializeElectronEmojis();
    void SetupChatLabel();
};
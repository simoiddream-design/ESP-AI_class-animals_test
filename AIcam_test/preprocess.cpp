#include "preprocess.h"

// Nearest-neighbor resize from the camera frame (RGB565, fb->width x fb->height)
// down to INPUT_WIDTH x INPUT_HEIGHT, converting to float RGB in [0,1].
// Output layout: HWC (height, width, channel), matching the model input [1,96,96,3].

bool ImagePreprocessor::convert(camera_fb_t *fb, int8_t *input) {

    if (fb == nullptr || input == nullptr) {
        return false;
    }

    // We expect RGB565 from the camera (see camera.cpp config).
    if (fb->format != PIXFORMAT_RGB565) {
        return false;
    }

    const int srcW = fb->width;
    const int srcH = fb->height;

    if (srcW <= 0 || srcH <= 0) {
        return false;
    }

    const uint16_t *src = (const uint16_t *)fb->buf;

    for (int y = 0; y < INPUT_HEIGHT; y++) {
        // マップ先（リサイズ用）のY座標計算
        int srcY = (y * srcH) / INPUT_HEIGHT;
        if (srcY >= srcH) srcY = srcH - 1;

        for (int x = 0; x < INPUT_WIDTH; x++) {
            int srcX = (x * srcW) / INPUT_WIDTH;
            if (srcX >= srcW) srcX = srcW - 1;

            // ピクセルデータの取得
            uint16_t pixel = src[srcY * srcW + srcX];

            // ESP32カメラのエンディアン（バイト）スワップ
            pixel = (pixel >> 8) | (pixel << 8);

            // 1. RGB565 から元のビット幅のまま抽出
            uint8_t r_raw = (pixel >> 11) & 0x1F; // 5bit (0~31)
            uint8_t g_raw = (pixel >> 5)  & 0x3F; // 6bit (0~63)
            uint8_t b_raw =  pixel        & 0x1F; // 5bit (0~31)

            // 2. 5bit/6bit を 8bit (0~255) の範囲に引き伸ばす（正規化の準備）
            uint8_t r_8bit = (r_raw * 255) / 31;
            uint8_t g_8bit = (g_raw * 255) / 63;
            uint8_t b_8bit = (b_raw * 255) / 31;

            int8_t r_int8 = (int8_t)(r_8bit - 128);
            int8_t g_int8 = (int8_t)(g_8bit - 128);
            int8_t b_int8 = (int8_t)(b_8bit - 128);

            
            *input++ = r_int8;
            *input++ = g_int8;
            *input++ = b_int8;
        }
    }
    return true;
}

 

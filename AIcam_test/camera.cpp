#include <Arduino.h>
#include "camera.h"
#include "camera_pins.h"
CameraClass Camera;

bool CameraClass::begin() {

    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;

    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;

    config.pixel_format = PIXFORMAT_RGB565;

    if (psramFound()) {

        config.frame_size = FRAMESIZE_QVGA;

        config.jpeg_quality = 12;

        config.fb_count = 2;

    }
    else {

        config.frame_size = FRAMESIZE_QQVGA;

        config.jpeg_quality = 20;

        config.fb_count = 1;

    }

    esp_err_t err = esp_camera_init(&config);

    if (err != ESP_OK) {

        Serial.printf("Camera Init Failed : 0x%x\n", err);

        return false;

    }

    sensor_t *sensor = esp_camera_sensor_get();

    sensor->set_brightness(sensor,0);
    sensor->set_contrast(sensor,0);
    sensor->set_saturation(sensor,0);
    sensor->set_whitebal(sensor,1);
    sensor->set_gain_ctrl(sensor,1);
    sensor->set_exposure_ctrl(sensor,1);
    sensor->set_hmirror(sensor,0);
    sensor->set_vflip(sensor,0);

    Serial.println("Camera initialized.");

    return true;

}

camera_fb_t* CameraClass::capture() {

    return esp_camera_fb_get();

}

void CameraClass::release(camera_fb_t *fb) {

    esp_camera_fb_return(fb);

}
#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include <esp_camera.h>

class CameraClass {

public:

    bool begin();

    camera_fb_t *capture();

    void release(camera_fb_t *fb);

};

extern CameraClass Camera;

#endif
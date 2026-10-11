#ifndef INFERENCE_H
#define INFERENCE_H
#include <Arduino.h>
#include <esp_camera.h>

class InferenceClass {

public:

    bool begin();
    bool run(camera_fb_t *fb, String &out_label, float &out_conf);

};

extern InferenceClass Inference;

#endif
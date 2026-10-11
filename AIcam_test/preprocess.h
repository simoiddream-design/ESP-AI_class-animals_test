#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <esp_camera.h>

#define INPUT_WIDTH     96
#define INPUT_HEIGHT    96
#define INPUT_CHANNELS  3

class ImagePreprocessor {

public:

    static bool convert(camera_fb_t *fb, int8_t *input);

};

#endif
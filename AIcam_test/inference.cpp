
#include <Arduino.h>
#include "inference.h"
#include "preprocess.h"
#include "labels.h"
#include "model_data.h"

// TensorFlow Lite Micro headers
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

InferenceClass Inference;

namespace {

    const tflite::Model      *model            = nullptr;
    tflite::MicroInterpreter *interpreter       = nullptr;
    TfLiteTensor             *input             = nullptr;

    constexpr int kTensorArenaSize = 800 * 1024;  // 800 KB
    uint8_t *tensor_arena = nullptr;

    // static な resolver をやめて、ポインタ型にします（スタックオーバーフロー対策）
    tflite::MicroMutableOpResolver<20> *resolver = nullptr;
}

bool InferenceClass::begin() {

    // PSRAMからのメモリ確保処理
    if (psramFound()) {
        tensor_arena = (uint8_t *)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM);
    } 
    if (tensor_arena == nullptr) {
        tensor_arena = (uint8_t *)malloc(kTensorArenaSize);
    }
    if (tensor_arena == nullptr) return false;

    // resolver も実行時に動的にメモリを確保します
    resolver = new tflite::MicroMutableOpResolver<20>();
    if (resolver == nullptr) {
        Serial.println("Failed to allocate resolver!");
        return false;
    }

    model = tflite::GetModel(model_tflite);

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("Model schema version %lu != supported %d\n",
                      (unsigned long)model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // 必要となる各オペレータの登録（INT8量子化用のQuantize/DequantizeやCONCATENATIONを網羅）
    resolver->AddConv2D();
    resolver->AddDepthwiseConv2D();
    resolver->AddFullyConnected();
    resolver->AddRelu();
    resolver->AddRelu6();
    resolver->AddReshape();
    resolver->AddSoftmax();
    resolver->AddAveragePool2D();
    resolver->AddMaxPool2D();
    resolver->AddMean();          
    resolver->AddAdd();
    resolver->AddPad();
    resolver->AddReduceMax();    
    resolver->AddLogistic();     
    resolver->AddMul();          
    resolver->AddConcatenation(); 
    resolver->AddSub();
    resolver->AddQuantize();
    resolver->AddDequantize();

    static tflite::MicroInterpreter static_interpreter(
        model, *resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    TfLiteStatus alloc_status = interpreter->AllocateTensors();
    if (alloc_status != kTfLiteOk) {
        Serial.println("AllocateTensors() failed.");
        return false;
    }

    input = interpreter->input(0);


    if (input->dims->size != 4 ||
        input->dims->data[1] != INPUT_HEIGHT ||
        input->dims->data[2] != INPUT_WIDTH  ||
        input->dims->data[3] != INPUT_CHANNELS ||
        input->type != kTfLiteInt8) {
        Serial.println("Unexpected input tensor shape/type.");
        Serial.printf("Got dims=%d [%d,%d,%d,%d] type=%d\n",
                      input->dims->size,
                      input->dims->data[0], input->dims->data[1],
                      input->dims->data[2], input->dims->data[3],
                      input->type);
        return false;
    }

    Serial.printf("Arena used: %u bytes\n",
                  (unsigned)interpreter->arena_used_bytes());
    Serial.println("TensorFlow Lite Micro initialized.");
    return true;
}


bool InferenceClass::run(camera_fb_t *fb, String &out_label, float &out_conf) {

    if (interpreter == nullptr || input == nullptr) {
        return false;
    }

    // カメラ画像を INT8 のバッファへ書き込み
    if (!ImagePreprocessor::convert(fb, input->data.int8)) {
        Serial.println("Preprocess failed.");
        return false;
    }

    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("Invoke failed.");
        return false;
    }

    TfLiteTensor *classes = nullptr;
    for (int i = 0; i < (int)interpreter->outputs_size(); i++) {
        TfLiteTensor *o = interpreter->output(i);
        int n = o->dims->data[o->dims->size - 1];
        if (n == NUM_CLASSES) {
            classes = o;
            break;
        }
    }

    if (classes == nullptr || classes->type != kTfLiteInt8) {
        Serial.println("Classification output not found or not INT8.");
        return false;
    }

    const int8_t *scores = classes->data.int8;

    int   best_idx   = 0;
    int8_t best_score = scores[0];
    for (int i = 1; i < NUM_CLASSES; i++) {
        if (scores[i] > best_score) {
            best_score = scores[i];
            best_idx   = i;
        }
    }

    float confidence = (best_score + 128) / 255.0f;

    out_label = String(labels[best_idx]);
    out_conf = confidence;


    Serial.printf("[Internal] Prediction: %-16s  confidence: %.2f\n", out_label.c_str(), out_conf);

    return true;
}
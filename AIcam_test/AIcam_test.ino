#include "camera.h"
#include "inference.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>


const char* ssid     = SSID;
const char* password = PW;


const char* server_mdns_hostname = hostname; 
const char* server_path = SP; 
const int server_port = port;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");

  
    if (!MDNS.begin("esp32cam")) {
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("mDNS responder started (esp32cam.local)");
    }
    Serial.println();
    Serial.println("================================");
    Serial.println("ESP32 AI Animal Classifier");
    Serial.println("================================");

    if (!Inference.begin()) {
        Serial.println("TensorFlow initialization failed.");
        while (1);
    }

    // その後にカメラを初期化する
    if (!Camera.begin()) {
        Serial.println("Camera initialization failed.");
        while (1);
    }
    Serial.println("System Ready");
}

void loop() {

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        delay(1000);
        return;
    }

    String detected_label = "";
    float confidence = 0.0f;


    if (Inference.run(fb, detected_label, confidence)) {
        
      
        if (confidence >= 0.40) {
            Serial.printf(">>> Confidence %.2f >= 0.40! Sending via Wi-Fi...\n", confidence);
            
            
            Serial.printf("Resolving mDNS hostname: %s.local ... ", server_mdns_hostname);
            IPAddress server_ip = MDNS.queryHost(server_mdns_hostname);
            
            if (server_ip.toString() == "0.0.0.0") {
                Serial.println("Failed! Host could not be resolved via mDNS.");
            } else {
                Serial.printf("Success! IP address is: %s\n", server_ip.toString().c_str());
                
            
                if (WiFi.status() == WL_CONNECTED) {
                    HTTPClient http;
               
                    String url = "http://" + server_ip.toString() + ":" + String(server_port) + server_path;
                    http.begin(url);
                    http.begin(url);
                    http.addHeader("Content-Type", "application/json");
                  
                    String jsonPayload = "{\"label\":\"" + detected_label + "\",\"confidence\":" + String(confidence, 2) + "}";
                    

                    int httpResponseCode = http.POST(jsonPayload);
                    
                    if (httpResponseCode > 0) {
                        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
                    } else {
                        Serial.printf("Error on sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
                    }
                    http.end();
                } else {
                    Serial.println("WiFi Disconnected. Cannot send data.");
                }
            }
        } else {
            Serial.printf("<<< Confidence %.2f < 0.40. Ignored.\n", confidence);
        }
    }


    esp_camera_fb_return(fb);
    

    delay(500); 
}
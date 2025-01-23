// CameraServer.h
#ifndef CAMERA_SERVER_H
#define CAMERA_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "esp_camera.h"
#include "camera_pins.h"


#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: %s\r\nContent-Length: %u\r\n\r\n";

static const char* JPG_CONTENT_TYPE = "image/jpeg";
static const char* BMP_CONTENT_TYPE = "image/x-windows-bmp";

// Function prototypes
void sendBMP(AsyncWebServerRequest* request);
void sendJpg(AsyncWebServerRequest* request);
void streamJpg(AsyncWebServerRequest* request);
void getCameraStatus(AsyncWebServerRequest* request);
void setCameraVar(AsyncWebServerRequest* request);

void initCamera();

typedef struct {
    camera_fb_t* fb;
    size_t index;
} camera_frame_t;


// Class declarations
class AsyncBufferResponse : public AsyncAbstractResponse {
private:
    uint8_t* _buf;
    size_t _len;
    size_t _index;
public:
    AsyncBufferResponse(uint8_t* buf, size_t len, const char* contentType);
    ~AsyncBufferResponse();
    bool _sourceValid() const override;
    size_t _fillBuffer(uint8_t* buf, size_t maxLen) override;
    size_t _content(uint8_t* buffer, size_t maxLen, size_t index);
};

class AsyncFrameResponse : public AsyncAbstractResponse {
private:
    camera_fb_t* fb;
    size_t _index;
public:
    AsyncFrameResponse(camera_fb_t* frame, const char* contentType);
    ~AsyncFrameResponse();
    bool _sourceValid() const override;
    size_t _fillBuffer(uint8_t* buf, size_t maxLen) override;
    size_t _content(uint8_t* buffer, size_t maxLen, size_t index);
};

class AsyncJpegStreamResponse : public AsyncAbstractResponse {
private:
    camera_frame_t _frame;
    size_t _index;
    size_t _jpg_buf_len;
    uint8_t* _jpg_buf;
    uint64_t lastAsyncRequest;
public:
    AsyncJpegStreamResponse();
    ~AsyncJpegStreamResponse();
    bool _sourceValid() const override;
    size_t _fillBuffer(uint8_t* buf, size_t maxLen) override;
    size_t _content(uint8_t* buffer, size_t maxLen, size_t index);
};
#endif

// CameraServer.cpp
#include "camera_server.h"

// Implementations of AsyncBufferResponse class
AsyncBufferResponse::AsyncBufferResponse(uint8_t* buf, size_t len, const char* contentType) {
    _buf = buf;
    _len = len;
    _callback = nullptr;
    _code = 200;
    _contentLength = _len;
    _contentType = contentType;
    _index = 0;
}

AsyncBufferResponse::~AsyncBufferResponse() {
    if (_buf != nullptr) {
        free(_buf);
    }
}

bool AsyncBufferResponse::_sourceValid() const {
    return _buf != nullptr;
}

size_t AsyncBufferResponse::_fillBuffer(uint8_t* buf, size_t maxLen) {
    size_t ret = _content(buf, maxLen, _index);
    if (ret != RESPONSE_TRY_AGAIN) {
        _index += ret;
    }
    return ret;
}

size_t AsyncBufferResponse::_content(uint8_t* buffer, size_t maxLen, size_t index) {
    memcpy(buffer, _buf + index, maxLen);
    if ((index + maxLen) == _len) {
        free(_buf);
        _buf = nullptr;
    }
    return maxLen;
}

AsyncFrameResponse::AsyncFrameResponse(camera_fb_t* frame, const char* contentType)
{
    _callback = nullptr;
    _code = 200;
    _contentLength = frame->len;
    _contentType = contentType;
    _index = 0;
    fb = frame;
}

AsyncFrameResponse::~AsyncFrameResponse()
{
    if (fb != nullptr) {
        esp_camera_fb_return(fb);
    }
}

bool AsyncFrameResponse::_sourceValid() const
{
    return fb != nullptr;
}

size_t AsyncFrameResponse::_fillBuffer(uint8_t* buf, size_t maxLen)
{
    size_t ret = _content(buf, maxLen, _index);
    if (ret != RESPONSE_TRY_AGAIN) {
        _index += ret;
    }
    return ret;
}

size_t AsyncFrameResponse::_content(uint8_t* buffer, size_t maxLen, size_t index)
{
    memcpy(buffer, fb->buf + index, maxLen);
    if ((index + maxLen) == fb->len) {
        esp_camera_fb_return(fb);
        fb = nullptr;
    }
    return maxLen;
}

AsyncJpegStreamResponse::AsyncJpegStreamResponse()
{
    _callback = nullptr;
    _code = 200;
    _contentLength = 0;
    _contentType = STREAM_CONTENT_TYPE;
    _sendContentLength = false;
    _chunked = true;
    _index = 0;
    _jpg_buf_len = 0;
    _jpg_buf = NULL;
    lastAsyncRequest = 0;
    memset(&_frame, 0, sizeof(camera_frame_t));
}

AsyncJpegStreamResponse::~AsyncJpegStreamResponse()
{
    if (_frame.fb) {
        if (_frame.fb->format != PIXFORMAT_JPEG) {
            free(_jpg_buf);
        }
        esp_camera_fb_return(_frame.fb);
    }
}

bool AsyncJpegStreamResponse::_sourceValid() const
{
    return true;
}

size_t AsyncJpegStreamResponse::_fillBuffer(uint8_t* buf, size_t maxLen)
{
    size_t ret = _content(buf, maxLen, _index);
    if (ret != RESPONSE_TRY_AGAIN) {
        _index += ret;
    }
    return ret;
}

size_t AsyncJpegStreamResponse::_content(uint8_t* buffer, size_t maxLen, size_t index)
{
    if (!_frame.fb || _frame.index == _jpg_buf_len) {
        if (index && _frame.fb) {
            uint64_t end = (uint64_t)micros();
            int fp = (end - lastAsyncRequest) / 1000;
            log_printf("Size: %uKB, Time: %ums (%.1ffps)\n", _jpg_buf_len / 1024, fp);
            lastAsyncRequest = end;
            if (_frame.fb->format != PIXFORMAT_JPEG) {
                free(_jpg_buf);
            }
            esp_camera_fb_return(_frame.fb);
            _frame.fb = NULL;
            _jpg_buf_len = 0;
            _jpg_buf = NULL;
        }
        if (maxLen < (strlen(STREAM_BOUNDARY) + strlen(STREAM_PART) + strlen(JPG_CONTENT_TYPE) + 8)) {
            //log_w("Not enough space for headers");
            return RESPONSE_TRY_AGAIN;
        }
        //get frame
        _frame.index = 0;

        _frame.fb = esp_camera_fb_get();
        if (_frame.fb == NULL) {
            log_e("Camera frame failed");
            return 0;
        }

        if (_frame.fb->format != PIXFORMAT_JPEG) {
            unsigned long st = millis();
            bool jpeg_converted = frame2jpg(_frame.fb, 80, &_jpg_buf, &_jpg_buf_len);
            if (!jpeg_converted) {
                log_e("JPEG compression failed");
                esp_camera_fb_return(_frame.fb);
                _frame.fb = NULL;
                _jpg_buf_len = 0;
                _jpg_buf = NULL;
                return 0;
            }
            log_i("JPEG: %lums, %uB", millis() - st, _jpg_buf_len);
        }
        else {
            _jpg_buf_len = _frame.fb->len;
            _jpg_buf = _frame.fb->buf;
        }

        //send boundary
        size_t blen = 0;
        if (index) {
            blen = strlen(STREAM_BOUNDARY);
            memcpy(buffer, STREAM_BOUNDARY, blen);
            buffer += blen;
        }
        //send header
        size_t hlen = sprintf((char*)buffer, STREAM_PART, JPG_CONTENT_TYPE, _jpg_buf_len);
        buffer += hlen;
        
        //send frame
        hlen = maxLen - hlen - blen;
        if (hlen > _jpg_buf_len) {
            maxLen -= hlen - _jpg_buf_len;
            hlen = _jpg_buf_len;
        }
        memcpy(buffer, _jpg_buf, hlen);
        _frame.index += hlen;
        return maxLen;
    }

    size_t available = _jpg_buf_len - _frame.index;
    if (maxLen > available) {
        maxLen = available;
    }
    memcpy(buffer, _jpg_buf + _frame.index, maxLen);
    _frame.index += maxLen;

    return maxLen;

}

// Function implementations
void sendBMP(AsyncWebServerRequest* request) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb == NULL) {
        log_e("Camera frame failed");
        request->send(501);
        return;
    }

    uint8_t* buf = NULL;
    size_t buf_len = 0;
    unsigned long st = millis();
    bool converted = frame2bmp(fb, &buf, &buf_len);
    log_i("BMP: %lums, %uB", millis() - st, buf_len);
    esp_camera_fb_return(fb);
    if (!converted) {
        request->send(501);
        return;
    }

    AsyncBufferResponse* response = new AsyncBufferResponse(buf, buf_len, BMP_CONTENT_TYPE);
    if (response == NULL) {
        log_e("Response alloc failed");
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void sendJpg(AsyncWebServerRequest* request) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb == NULL) {
        log_e("Camera frame failed");
        request->send(501);
        return;
    }

    if (fb->format == PIXFORMAT_JPEG) {
        AsyncFrameResponse* response = new AsyncFrameResponse(fb, JPG_CONTENT_TYPE);
        if (response == NULL) {
            log_e("Response alloc failed");
            request->send(501);
            return;
        }
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }

    size_t jpg_buf_len = 0;
    uint8_t* jpg_buf = NULL;
    unsigned long st = millis();
    bool jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
    esp_camera_fb_return(fb);
    if (!jpeg_converted) {
        log_e("JPEG compression failed: %lu", millis());
        request->send(501);
        return;
    }
    log_i("JPEG: %lums, %uB", millis() - st, jpg_buf_len);

    AsyncBufferResponse* response = new AsyncBufferResponse(jpg_buf, jpg_buf_len, JPG_CONTENT_TYPE);
    if (response == NULL) {
        log_e("Response alloc failed");
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void streamJpg(AsyncWebServerRequest* request) {
    AsyncJpegStreamResponse* response = new AsyncJpegStreamResponse();
    if (!response) {
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void getCameraStatus(AsyncWebServerRequest* request) {
    static char json_response[1024];

    sensor_t* s = esp_camera_sensor_get();
    if (s == NULL) {
        request->send(501);
        return;
    }
    char* p = json_response;
    *p++ = '{';

    p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p += sprintf(p, "\"quality\":%u,", s->status.quality);
    p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p += sprintf(p, "\"awb\":%u,", s->status.awb);
    p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p += sprintf(p, "\"aec\":%u,", s->status.aec);
    p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p += sprintf(p, "\"denoise\":%u,", s->status.denoise);
    p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p += sprintf(p, "\"agc\":%u,", s->status.agc);
    p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p += sprintf(p, "\"colorbar\":%u", s->status.colorbar);
    *p++ = '}';
    *p++ = 0;

    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", json_response);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void setCameraVar(AsyncWebServerRequest* request) {
    if (!request->hasArg("var") || !request->hasArg("val")) {
        request->send(404);
        return;
    }
    String var = request->arg("var");
    const char* variable = var.c_str();
    int val = atoi(request->arg("val").c_str());

    sensor_t* s = esp_camera_sensor_get();
    if (s == NULL) {
        request->send(501);
        return;
    }

    int res = 0;
    if (!strcmp(variable, "framesize")) res = s->set_framesize(s, (framesize_t)val);
    else if (!strcmp(variable, "quality")) res = s->set_quality(s, val);
    else if (!strcmp(variable, "contrast")) res = s->set_contrast(s, val);
    else if (!strcmp(variable, "brightness")) res = s->set_brightness(s, val);
    else if (!strcmp(variable, "saturation")) res = s->set_saturation(s, val);
    else if (!strcmp(variable, "sharpness")) res = s->set_sharpness(s, val);
    else if (!strcmp(variable, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if (!strcmp(variable, "colorbar")) res = s->set_colorbar(s, val);
    else if (!strcmp(variable, "awb")) res = s->set_whitebal(s, val);
    else if (!strcmp(variable, "agc")) res = s->set_gain_ctrl(s, val);
    else if (!strcmp(variable, "aec")) res = s->set_exposure_ctrl(s, val);
    else if (!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);
    else if (!strcmp(variable, "vflip")) res = s->set_vflip(s, val);
    else if (!strcmp(variable, "awb_gain")) res = s->set_awb_gain(s, val);
    else if (!strcmp(variable, "agc_gain")) res = s->set_agc_gain(s, val);
    else if (!strcmp(variable, "aec_value")) res = s->set_aec_value(s, val);
    else if (!strcmp(variable, "aec2")) res = s->set_aec2(s, val);
    else if (!strcmp(variable, "denoise")) res = s->set_denoise(s, val);
    else if (!strcmp(variable, "dcw")) res = s->set_dcw(s, val);
    else if (!strcmp(variable, "bpc")) res = s->set_bpc(s, val);
    else if (!strcmp(variable, "wpc")) res = s->set_wpc(s, val);
    else if (!strcmp(variable, "raw_gma")) res = s->set_raw_gma(s, val);
    else if (!strcmp(variable, "lenc")) res = s->set_lenc(s, val);
    else if (!strcmp(variable, "special_effect")) res = s->set_special_effect(s, val);
    else if (!strcmp(variable, "wb_mode")) res = s->set_wb_mode(s, val);
    else if (!strcmp(variable, "ae_level")) res = s->set_ae_level(s, val);

    else {
        log_e("unknown setting %s", var.c_str());
        request->send(404);
        return;
    }
    log_d("Got setting %s with value %d. Res: %d", var.c_str(), val, res);

    AsyncWebServerResponse* response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void initCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
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
    config.xclk_freq_hz = 8000000;
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        log_e("ERROR : Camera init failed with error 0x%x\n", err);
        return;
    }
    else {
        log_d("Camera init OK");
    }

    sensor_t* s = esp_camera_sensor_get();
    log_d("Sensor PID : %d\n", s->id.PID);

    // Make sure we can read the file system
    if (!SPIFFS.begin(true)) {
        Serial.println("Error mounting SPIFFS");
        while (1);
    }
}

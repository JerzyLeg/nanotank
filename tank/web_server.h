#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include "camera_server.h"
#include "motor_control.h"

extern AsyncWebServer server;
extern WebSocketsServer webSocket;

void onIndexRequest(AsyncWebServerRequest* request);
void onCSSRequest(AsyncWebServerRequest* request);
void onPageNotFound(AsyncWebServerRequest* request);

void setupWebServer();
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length);

#endif

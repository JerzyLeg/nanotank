#include "web_server.h"

AsyncWebServer server(80);
WebSocketsServer webSocket(1337);


void onIndexRequest(AsyncWebServerRequest* request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/index.html", "text/html");
}

void onCSSRequest(AsyncWebServerRequest* request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());
    request->send(SPIFFS, "/style.css", "text/css");
}

void onPageNotFound(AsyncWebServerRequest* request)
{
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());
    request->send(404, "text/plain", "Not found");
}

void setupWebServer() {
    // SPIFFS setup and server routes
    SPIFFS.begin();
    
    // On HTTP request for root, provide index.html file
    server.on("/", HTTP_GET, onIndexRequest);

    // On HTTP request for style sheet, provide style.css
    server.on("/style.css", HTTP_GET, onCSSRequest);

    // Handle requests for pages that do not exist
    server.onNotFound(onPageNotFound);

    initCamera();

    // Handle camera streaming
    //server.on("/bmp", HTTP_GET, sendBMP);
    //server.on("/capture", HTTP_GET, sendJpg);
    server.on("/stream", HTTP_GET, streamJpg);
    //server.on("/control", HTTP_GET, setCameraVar);
    //server.on("/status", HTTP_GET, getCameraStatus);

    // WebSocket setup
    webSocket.onEvent(onWebSocketEvent);
    webSocket.begin();
    server.begin();
}
char msg_buffer[20];
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
    case WStype_CONNECTED:
        Serial.printf("Client [%u] connected\n", client_num);

        // Send the current motor states to the newly connected client
        sprintf(msg_buffer, "motorAforward:%d", motorAForward);
        webSocket.sendTXT(client_num, msg_buffer);
        
        sprintf(msg_buffer, "motorAbackward:%d", motorABackward);
        webSocket.sendTXT(client_num, msg_buffer);
        
        sprintf(msg_buffer, "motorBforward:%d", motorBForward);
        webSocket.sendTXT(client_num, msg_buffer);
        
        sprintf(msg_buffer, "motorBbackward:%d", motorBBackward);
        webSocket.sendTXT(client_num, msg_buffer);
        
        break;
    case WStype_DISCONNECTED:
        Serial.printf("Client [%u] disconnected\n", client_num);
        break;
    case WStype_TEXT:
        Serial.printf("[%u] Received text: %s\n", client_num, payload);
        handleMotorCommands(payload, client_num, webSocket);
        break;
    default:
        break;
    }
}

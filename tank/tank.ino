#include "motor_control.h"
#include "web_server.h"

const char* ssid = "WiFi_ssid";
const char* password = "WiFi_password";

void setup() {
  
  // Call initialization functions from other files
  setupMotors();
  
  // Start Serial port
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Start access point
  //WiFi.mode(WIFI_AP);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print our IP address
  /*Serial.println();
  Serial.println("AP running");
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());*/

  setupWebServer();

  Serial.print("Tank Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {

  // Look for and handle WebSocket data
  webSocket.loop();
}

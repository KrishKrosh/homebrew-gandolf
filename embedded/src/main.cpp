#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include <Arduino.h>
#include "secrets.h" // Include secrets header file
#include "ServoController.h" // Include our Servo Controller
#include "ButtonHandler.h" // Include our Button Handler

// Flash button pin (typically GPIO0 on most ESP32 dev boards)
#define FLASH_BUTTON_PIN 0
 
// Use credentials from secrets.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Uncomment to use alternative credentials
// const char* ssid = ALT_WIFI_SSID;
// const char* password = ALT_WIFI_PASSWORD;
 
WebServer server(80);

// Create a ServoController instance
ServoController servoCtrl;

// Create a ButtonHandler instance for the flash button
ButtonHandler flashButton(FLASH_BUTTON_PIN);

// WiFi connection state management
enum WiFiConnectionState {
  WIFI_DISCONNECTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED
};

WiFiConnectionState wifiState = WIFI_DISCONNECTED;
unsigned long wifiConnectionStartTime = 0;
unsigned long wifiConnectionAttempt = 0;
const unsigned long WIFI_CONNECTION_TIMEOUT = 20000; // 20 seconds timeout
const unsigned long WIFI_RETRY_INTERVAL = 30000;     // Try reconnecting every 30 seconds
bool otaInitialized = false;

// Function to validate the API key
bool isValidApiKey() {
  // Check if API key is provided as a URL parameter
  if (server.hasArg("key")) {
    if (server.arg("key") == API_KEY) {
      return true;
    }
  }
  
  // Check if API key is provided in the Authorization header
  if (server.hasHeader("Authorization")) {
    String authHeader = server.header("Authorization");
    // Check if it starts with "Bearer " and then matches our API key
    if (authHeader.startsWith("Bearer ") && authHeader.substring(7) == API_KEY) {
      return true;
    }
    // Also check for direct API key in the header
    if (authHeader == API_KEY) {
      return true;
    }
  }

  // If no valid API key found
  return false;
}

// Add CORS headers to responses
void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.sendHeader("Access-Control-Max-Age", "86400"); // 24 hours cache for preflight requests
}

// Handle unauthorized access
void handleUnauthorized() {
  addCorsHeaders(); // Add CORS headers even for unauthorized responses
  server.send(401, "text/plain", "Unauthorized: Invalid or missing API key");
}

// Handle preflight OPTIONS requests
void handleOptions() {
  addCorsHeaders();
  server.send(200, "text/plain", "");
}

// Function to handle button press - will open both doors
void handleButtonPress() {
  Serial.println("Button press detected - opening both doors");
  servoCtrl.openBothDoors();
}

// Initialize OTA and web server
void setupOTAAndServer() {
  if (otaInitialized) return;

  // Set up mDNS responder
  if (!MDNS.begin("gandalf")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  }

  // ArduinoOTA setup
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("gandalf");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  
  // OTA callbacks
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA service started");
 
  // Set up web server endpoints
  server.on("/", []() {
    addCorsHeaders(); // Add CORS headers
    server.send(200, "text/plain", "Hi! This is Gandalf Door Controller.");
  });

  // Handle OPTIONS method for preflight requests
  server.on("/openFirstDoor", HTTP_OPTIONS, handleOptions);
  server.on("/openSecondDoor", HTTP_OPTIONS, handleOptions);
  server.on("/openBothDoors", HTTP_OPTIONS, handleOptions);

  server.on("/openFirstDoor", []() {
    // Check if the request has a valid API key
    if (!isValidApiKey()) {
      handleUnauthorized();
      return;
    }
    
    addCorsHeaders(); // Add CORS headers
    server.send(200, "text/plain", "Wait ~10 seconds for the first door to open");
    servoCtrl.openFirstDoor();
  });

  server.on("/openSecondDoor", []() {
    // Check if the request has a valid API key
    if (!isValidApiKey()) {
      handleUnauthorized();
      return;
    }
    
    addCorsHeaders(); // Add CORS headers
    server.send(200, "text/plain", "Wait ~10 seconds for the second door to open");
    servoCtrl.openSecondDoor();
  });

  server.on("/openBothDoors", []() {
    // Check if the request has a valid API key
    if (!isValidApiKey()) {
      handleUnauthorized();
      return;
    }
    
    addCorsHeaders(); // Add CORS headers
    server.send(200, "text/plain", "Wait ~20 seconds for both doors to open");
    servoCtrl.openBothDoors();
  });

  server.begin();
  Serial.println("HTTP server started");
  
  otaInitialized = true;
}

// Start WiFi connection process
void startWiFiConnection() {
  if (wifiState == WIFI_CONNECTING) return;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  
  wifiState = WIFI_CONNECTING;
  wifiConnectionStartTime = millis();
  wifiConnectionAttempt++;
}

// Check WiFi connection status
void checkWiFiConnection() {
  if (wifiState == WIFI_CONNECTING) {
    // Check if connected
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      
      wifiState = WIFI_CONNECTED;
      setupOTAAndServer();
    } 
    // Check for timeout
    else if (millis() - wifiConnectionStartTime > WIFI_CONNECTION_TIMEOUT) {
      Serial.println();
      Serial.println("WiFi connection timeout. Will retry later.");
      
      wifiState = WIFI_FAILED;
    }
    // Still connecting
    else {
      if (millis() % 1000 < 10) { // Print a dot roughly every second
        Serial.print(".");
      }
    }
  }
  // If connection failed and it's time to retry
  else if (wifiState == WIFI_FAILED && 
          (millis() - wifiConnectionStartTime) > WIFI_RETRY_INTERVAL) {
    Serial.println("Retrying WiFi connection...");
    startWiFiConnection();
  }
  // If we're connected but lost connection
  else if (wifiState == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    wifiState = WIFI_DISCONNECTED;
    startWiFiConnection();
  }
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("Gandalf Door Controller starting...");

  // Initialize the servo controller
  servoCtrl.begin();
  
  // Initialize the button handler and set the callback function
  flashButton.begin();
  flashButton.setOnPressCallback(handleButtonPress);
  Serial.println("Flash button configured to open both doors");

  // Start WiFi connection process asynchronously
  startWiFiConnection();
  
  Serial.println("Gandalf is ready! (Button functions available even without WiFi)");
}
 
void loop(void) {
  // Check WiFi connection status
  checkWiFiConnection();
  
  // Handle OTA if WiFi is connected
  if (wifiState == WIFI_CONNECTED) {
    ArduinoOTA.handle();
    server.handleClient();
  }
  
  // Always check for button press, regardless of WiFi state
  flashButton.handle();
}

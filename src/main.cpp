#include <Arduino.h>
#include <TFT_eSPI.h> // Include TFT library for TTGO display
#include <WiFi.h>
#include <esp_now.h>

// Define constants for digital pin numbers
const int carButtonPin = 37;

// Joystick data structure (simplified)
typedef struct {
    int16_t carX;
    int16_t carY;
    bool carButton;
} JoystickData;

// Update pin assignments to use ADC1 pins only
const int CAR_X_PIN = 36; // ADC1
const int CAR_Y_PIN = 39; // ADC1

JoystickData joystickData;

// MAC address of the receiver
uint8_t receiverMac[] = {0xD0, 0xEF, 0x76, 0xEE, 0xFD, 0xA4}; // Replace with the receiver's actual MAC address

// ESP-NOW send callback function
const char* getStatusString(esp_now_send_status_t status) {
    switch (status) {
        case ESP_NOW_SEND_SUCCESS:
            return "Success";
        case ESP_NOW_SEND_FAIL:
            return "Fail";
        default:
            return "Unknown";
    }
}

// Callback function to handle the status of data sent via ESP-NOW
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Data send status: ");
    Serial.println(getStatusString(status));
}

// Function to print joystick data
void printJoystickData() {
    Serial.print("Joystick X: ");
    Serial.println(joystickData.carX);
    Serial.print("Joystick Y: ");
    Serial.println(joystickData.carY);
    Serial.print("Car Button: ");
    Serial.println(joystickData.carButton);
}

// TFT display setup
TFT_eSPI tft = TFT_eSPI(); // Create an instance of the TFT_eSPI class

// Function to initialize and set up the TFT display
void setupDisplay() {
    tft.init();
    tft.setRotation(1); // Landscape orientation
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Joystick Ready", 10, 10);
}

// Setup function to initialize the hardware and communication protocols
void setup() {
    Serial.begin(115200);

    // Initialize Wi-Fi in station mode
    WiFi.mode(WIFI_STA);
    Serial.println(WiFi.macAddress());

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register the send callback function
    esp_now_register_send_cb(onDataSent);

    // Add the receiver peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0;  // Match the receiver's channel
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    // Print receiver MAC for verification
    Serial.printf("Receiver MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  receiverMac[0], receiverMac[1], receiverMac[2],
                  receiverMac[3], receiverMac[4], receiverMac[5]);

    Serial.println("Transmitter ready.");

    // Configure button pin as input with internal pull-up resistor
    pinMode(carButtonPin, INPUT_PULLUP);

    // Initialize the TFT display
    setupDisplay();
}

// Main loop to read joystick values, button states, and send data
void loop() {
    // Read joystick values
    joystickData.carX = analogRead(CAR_X_PIN);
    joystickData.carY = analogRead(CAR_Y_PIN);

    // Read button state for the car
    joystickData.carButton = digitalRead(carButtonPin) == LOW; // Assuming active low button

    // Send joystick data to receiver
    static unsigned long lastSendTime = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastSendTime >= 100) {
        lastSendTime = currentMillis;
        esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&joystickData, sizeof(joystickData));

        // Display transmission status on the TFT screen
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Joystick Data:", 10, 10);
        tft.printf("carX: %d\ncarY: %d\n",
                   joystickData.carX, joystickData.carY);

        if (result == ESP_OK) {
            tft.drawString("Data Sent: Success", 10, 90);
        } else {
            tft.drawString("Data Sent: Fail", 10, 90);
            tft.printf("Error code: %d", result);
        }
    }

    // Avoid flooding the serial monitor
    delay(100);
    printJoystickData();

    // Add a delay to avoid flooding the serial output
    delay(1000);
}

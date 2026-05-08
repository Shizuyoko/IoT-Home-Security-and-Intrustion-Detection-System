#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- PIN DEFINITIONS ---
const int PIR_PIN = 12;
const int buzzerPin = 21;

// --- RGB LED PINS (inverted logic: HIGH = ON) ---
const int LED_R = 4;    // Red channel
const int LED_G = 22;   // Green channel
const int LED_B = 16;   // Blue channel

// --- KEYPAD SETUP ---
const byte ROWS = 4;
const byte COLS = 4;

// --- DOOR MAGNET SETUP
const int DOOR_SENSOR_PIN = 32;

// --- Wi-Fi ACCESS POINT SETTINGS ---
const char* apSSID = "AlarmSystem";
const char* apPassword = "12345678";

// --- Wi-Fi STATION SETTINGS (ESP32 CONNECTS TO ROUTER) ---
const char* ssid     = "eir83427927";
const char* password = "3mU2cKFueD";

// --- BACKEND API SETTINGS ---
const char* apiUrl = "https://true-lion-mansion.loca.lt/api/events/";
LiquidCrystal lcd(14, 13, 27, 26, 25, 33);

bool alarmActive = false;
bool alarmLatched = false;
int lastMotionState = LOW;
int lastDoorState = HIGH;

// --- ALARM CONTROL VARIABLES ---
bool systemArmed = true;
String enteredCode = "";
String correctCode = "1234";

unsigned long lastMotionTime = 0;

// --- SIREN FUNCTION (4 cycles) ---
void playSiren() {

    // LED red while siren is playing
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, LOW);

    // Play 4 siren cycles
    for (int cycle = 0; cycle < 4; cycle++) {

        // Rising tone
        for (int freq = 600; freq <= 1600; freq += 10) {
            ledcWriteTone(0, freq);
            delay(5);
        }

        // Falling tone
        for (int freq = 1600; freq >= 600; freq -= 10) {
            ledcWriteTone(0, freq);
            delay(5);
        }
    }

    // Stop buzzer
    ledcWriteTone(0, 0);

    // Restore LED ONLY if the door is closed
    if (digitalRead(DOOR_SENSOR_PIN) == LOW) {
        if (systemArmed) {
            digitalWrite(LED_R, LOW);
            digitalWrite(LED_G, HIGH);
            digitalWrite(LED_B, LOW);
        } else {
            digitalWrite(LED_R, HIGH);
            digitalWrite(LED_G, HIGH);
            digitalWrite(LED_B, LOW);
        }
    }
}

void chirp() {
    ledcWriteTone(0, 2000);
    delay(80);
    ledcWriteTone(0, 0);
    delay(80);
}

void playArmChirp() {
    chirp();
    chirp();   // two chirps for ARM
}

void playDisarmChirp() {
    chirp();   // one chirp for DISARM
}


char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

byte rowPins[ROWS] = {15, 2, 0, 16};
byte colPins[COLS] = {17, 5, 18, 19};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void sendAlertToBackend(String eventType)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected");
        return;
    }

    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{"
                         "\"device_id\":\"esp32_lab_01\","
                         "\"event_type\":\"" + eventType + "\","
                         "\"location\":\"lab\""
                         "}";

    Serial.println("Sending payload: " + jsonPayload);

    http.useHTTP10(true);
    int httpCode = http.POST(jsonPayload);
    String response = http.getString();

    Serial.println("HTTP Response code: " + String(httpCode));
    Serial.println("Response: " + response);

    http.end();
}

void triggerCameraSnapshot() {
    HTTPClient http;
    http.begin("http://192.168.1.51:81/take_snapshot"); // ESP32-CAM snapshot endpoint
    int code = http.GET();
    Serial.println("Snapshot trigger response: " + String(code));
    http.end();
}


void setup()
{
    Serial.begin(115200);
    delay(200);

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to WiFi");
    }

    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("LCD Connected!");
    lcd.setCursor(0, 1);
    lcd.print("Hello there...");

    ledcSetup(0, 2000, 8);
    ledcAttachPin(buzzerPin, 0);

    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    pinMode(PIR_PIN, INPUT);

    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, LOW);

    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

    Serial.println("System initialized (Sensor mode ACTIVATED)");
}

void loop()
{
    char key = keypad.getKey();

    if (key)
    {
        Serial.print("Key pressed: ");
        Serial.println(key);

        if (key == '#')
        {
            if (enteredCode == correctCode)
            {
                systemArmed = !systemArmed;

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(systemArmed ? "SYSTEM ARMED" : "SYSTEM DISARMED");

                Serial.println(systemArmed ? "System is ARMED" : "System is DISARMED");

                if (systemArmed)
                {
                    digitalWrite(LED_R, LOW);
                    digitalWrite(LED_G, HIGH);
                    digitalWrite(LED_B, LOW);

                    playArmChirp();
                    sendAlertToBackend("system_armed");
                }
                else
                {
                    digitalWrite(LED_R, HIGH);
                    digitalWrite(LED_G, HIGH);
                    digitalWrite(LED_B, LOW);

                    playDisarmChirp();
                    sendAlertToBackend("system_disarmed");
                }
            }
            else
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("INVALID CODE");
                Serial.println("Invalid code entered!");
            }

            enteredCode = "";
        }
        else if (key == '*')
        {
            enteredCode = "";
            lcd.clear();
            lcd.print("CODE CLEARED...");
        }
        else
        {
            enteredCode += key;

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ENTER CODE:");
            lcd.setCursor(0, 1);
            lcd.print(enteredCode);
        }
    }

    int motion = digitalRead(PIR_PIN);
    int doorState = digitalRead(DOOR_SENSOR_PIN);


    // --- MOTION TRIGGER ---
    if (systemArmed && enteredCode == "" && motion == HIGH && lastMotionState == LOW)
    {
        // Debounce: only allow snapshot every 5 seconds
        if (millis() - lastMotionTime > 5000) {
            lastMotionTime = millis();

            alarmLatched = true;

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("MOTION DETECTED!");
            lcd.setCursor(0, 1);
            lcd.print("ALARM ACTIVATED!");

            Serial.println("Motion detected!");

            // 1. Trigger snapshot BEFORE siren
            triggerCameraSnapshot();

            // 2. Give CAM time to capture & upload
            delay(500);

            // 3. Send event payload to Evan's backend
             sendAlertToBackend("motion");

            // 4. Activate siren
            playSiren();
        }
    }

    // --- PREVENT RESET WHILE ALARM LATCHED ---
    if (!alarmLatched && motion == LOW && lastMotionState == HIGH)
    {
        Serial.println("Motion ended");

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(systemArmed ? "SYSTEM ARMED" : "SYSTEM DISARMED");

        lcd.setCursor(0, 1);
        if (systemArmed)
        {
            lcd.print("READY...");
        }
        else
        {
            lcd.print("             "); // blank line when disarmed
        }
    }

    // --- DOOR OPENED TRIGGER ---
    if (systemArmed && enteredCode == "" && doorState == HIGH && lastDoorState == LOW)
    {
        alarmLatched = true;

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("DOOR OPENED!");
        lcd.setCursor(0, 1);
        lcd.print("ALARM ACTIVATED!");

        Serial.println("Door opened!");

        sendAlertToBackend("door_opened");
        playSiren();
    }

    // --- DOOR CLOSED (RESET ALARM) ---
    if (doorState == LOW && lastDoorState == HIGH)
    {
        Serial.println("Door closed");

        alarmLatched = false;

        // Restore LED to green (system armed)
        if (systemArmed)
        {
            digitalWrite(LED_R, LOW);
            digitalWrite(LED_G, HIGH);
            digitalWrite(LED_B, LOW);
        } else
        {
            digitalWrite(LED_R, HIGH);
            digitalWrite(LED_G, HIGH);
            digitalWrite(LED_B, LOW);
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(systemArmed ? "SYSTEM ARMED" : "SYSTEM DISARMED");

        lcd.setCursor(0, 1);
        if (systemArmed)
        {
            lcd.print("READY...");
        }
        else
        {
        lcd.print("             "); // Bottom LCD line blank in DISARMED mode
        }
    }

    lastMotionState = motion;
    lastDoorState = doorState;

    // --- CAM CONNECTIVITY CHECK ( every 3 seconds) ---
    static unsigned long lastCamCheck = 0;
    if (millis() - lastCamCheck > 60000) { // check for CAM connectivity
        lastCamCheck = millis();           // every minute.

        WiFiClient client;
        if (client.connect("192.168.1.51", 81)) {
            Serial.println("CAM is reachable!");
            client.stop();
        } else {
            Serial.println("CAM is NOT reachable!");
        }
    }
    delay(50);
}
#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#define WS_RECONNECT_INTERVAL (1 * 1000)
#define PING_INTERVAL (30 * 1000)
#define MSG_INIT_INTERVAL (5 * 1000)
#define NEW_MSG_BLINK_TIME (5 * 1000)

// TODO: CHANGE ALL OF THESE
#define LED_NEW_MSG LED_BUILTIN
#define LED_WIFI LED_BUILTIN
#define LED_POWER LED_BUILTIN

WiFiManager wm;

using namespace websockets;

WebsocketsClient client;

unsigned long lastMessageEpoch = 0;
unsigned long lastMessageMillis = -1;
unsigned long lastPing = 0;
unsigned long lastSend = 0;

// This is set to false when opening a new connection
// and then true when receiving any message
bool listeningToMessages = false;

void onMessageCallback(const WebsocketsMessage &message) {
    const String txt = message.data();
    Serial.print("[D] Got ws text: ");
    Serial.println(txt);
    if (txt.length() > 16 && txt[0] == 'i') {
        Serial.println("[D] Looks like a message event - parsing for timestamp...");
        listeningToMessages = true;
        const int firstIdx = txt.indexOf(':');
        if (firstIdx < 0) {
            Serial.println("[W] First colon not found :( aborting");
            return;
        }
        const int secondIdx = txt.indexOf(':', firstIdx + 1);
        if (secondIdx < 0) {
            Serial.println("[W] Second colon not found :( aborting");
            return;
        }
        const unsigned long epoch = txt.substring(firstIdx + 1, secondIdx).toInt();
        if (epoch > lastMessageEpoch) {
            Serial.print("[I] New message! Updating timestamp to ");
            Serial.println(epoch);
            lastMessageEpoch = epoch;
            lastMessageMillis = millis();
        }
    }
}

void onEventsCallback(const WebsocketsEvent event, const String &data) {
    switch (event) {
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("[D] WS connection opened");
            listeningToMessages = false;
            break;
        case WebsocketsEvent::ConnectionClosed:
            Serial.println("[D] WS connection closed");
            break;
        case WebsocketsEvent::GotPing:
            Serial.println("[D] WS got a Ping!");
            break;
        case WebsocketsEvent::GotPong:
            Serial.println("[D] WS got a Pong!");
            break;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_NEW_MSG, OUTPUT);
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_POWER, OUTPUT);
    digitalWrite(LED_NEW_MSG, 0);
    digitalWrite(LED_WIFI, 1); // to show that connecting to wifi (initially)
    digitalWrite(LED_POWER, 1); // to show power

    wm.setConfigPortalBlocking(false);
    wm.setConnectTimeout(30);
    wm.setSaveConnectTimeout(30);
    wm.setConfigPortalTimeout(300);
    wm.autoConnect("LUZ-Czat-Mrugacz");

    client.onMessage(onMessageCallback);
    client.onEvent(onEventsCallback);
    client.addHeader("Origin", "https://st.chatango.com");
    client.setInsecure();

    lastSend = millis();
    lastPing = millis();
}

void loop() {
    client.poll();
    wm.process();

    // ====== Connection stuff ======
    if (WiFi.getMode() != WIFI_STA || !WiFi.isConnected()) {
        digitalWrite(LED_WIFI, 1);
        return;
    }
    digitalWrite(LED_WIFI, 0);
    if (!client.available() && (millis() - lastPing > WS_RECONNECT_INTERVAL)) {
        lastPing = millis();
        if (client.connect("wss://s13.chatango.com:8081")) {
            Serial.println("[I] Connected to Kutango!");
        } else {
            Serial.println("[E] Znowu sie nie da połączyć z zasranym Kutango!");
        }
    }
    if (client.available()) {
        // Send init 'v' message if no messages yet
        if (!listeningToMessages && (millis() - lastSend > MSG_INIT_INTERVAL)) {
            lastSend = millis();
            Serial.println("[D] Sending");
            client.sendBinary("v", 2);
            client.sendBinary("bauth:akademickieradioluz:::", 29);
        }
        // Periodic ping
        if (millis() - lastPing > PING_INTERVAL) {
            lastPing = millis();
            client.sendBinary("", 1);
        }
    }
    // ====== Connection stuff ======

    // ====== Visuals ======
    if (millis() - lastMessageMillis < NEW_MSG_BLINK_TIME) {
        // TODO: Fancy blinking here
        digitalWrite(LED_NEW_MSG, 0);
    } else {
        digitalWrite(LED_NEW_MSG, 1);
    }
    // ====== Visuals ======
}

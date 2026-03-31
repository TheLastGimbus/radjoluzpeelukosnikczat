#include <Arduino.h>
#include <ArduinoWebsockets.h>

const char *ssid = "Pan Tadeusz i Spolka";
const char *password = "adammickiewicz";

#define PING_INTERVAL (10 * 1000)

using namespace websockets;

WebsocketsClient client;

unsigned long lastMessageEpoch = 0;
unsigned long lastMessageMillis = 0;
unsigned long lastPing = 0;
unsigned long lastSend = 0;

void onMessageCallback(const WebsocketsMessage &message) {
    const String txt = message.data();
    Serial.print("[D] Got ws text: ");
    Serial.println(txt);
    if (txt.length() > 16 && txt[0] == 'i') {
        Serial.println("[D] Looks like a message event - parsing for timestamp...");
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

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    client.onMessage(onMessageCallback);
    client.onEvent(onEventsCallback);
    client.addHeader("Origin", "https://st.chatango.com");
    client.setInsecure();
    bool connected = client.connect("wss://s13.chatango.com:8081");
    if (connected) {
        Serial.println("Connected to WebSocket server!");
    } else {
        Serial.println("Connection failed!");
    }
    lastSend = millis();
}

static constexpr char arr[1] = "";

void loop() {
        client.poll();

    // Poll for WebSocket events
    if (client.available()) {

        // Periodic ping
        // if (millis() - lastPing > PING_INTERVAL) {
        //     lastPing = millis();
        //     client.ping();
        // }


        // Send a message every 5 seconds
        if (millis() - lastSend > 5000) {
            Serial.println("[D] Sending");
            lastSend = millis();
            client.send("v");
            client.sendBinary(arr);
            client.sendBinary("bauth:akademickieradioluz:::");
        }
    }
}

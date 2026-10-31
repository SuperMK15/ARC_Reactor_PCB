#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "reactor_fx.h"

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
ReactorFX reactor;

String apSSID;
uint32_t lastTelemetryMs = 0;

String buildDynamicSSID() {
    uint64_t chipid = ESP.getEfuseMac();
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
        mac[i] = (chipid >> (8 * i)) & 0xFF;
    }
    char uuid[9];
    snprintf(uuid, sizeof(uuid), "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
    return String(AP_SSID_PREFIX) + String(uuid);
}

String RGBToHex(const RGBColor& c) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
    return String(buf);
}

void broadcastState(bool includeFullConfig = false) {
    if (ws.count() == 0) return;

    JsonDocument doc;
    const ReactorState& st = reactor.getState();

    doc["type"] = includeFullConfig ? "sync" : "telemetry";
    doc["ssid"] = apSSID;
    doc["power"] = st.powerOn;
    doc["mode"] = (uint8_t)st.mode;
    doc["brightness"] = st.brightness;
    doc["speed"] = st.speed;
    doc["coreColor"] = RGBToHex(st.coreColor);
    doc["ringColor"] = RGBToHex(st.ringColor);
    doc["unibeam"] = st.unibeamActive;

    String hexLeds[NUM_LEDS_TOTAL];
    reactor.getPixelHexArray(hexLeds);
    JsonArray ledsArr = doc["leds"].to<JsonArray>();
    for (uint8_t i = 0; i < NUM_LEDS_TOTAL; i++) ledsArr.add(hexLeds[i]);

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) return;

    JsonDocument doc;
    if (deserializeJson(doc, data, len)) return;

    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "setMode") == 0) {
        reactor.setMode((ReactorMode)(uint8_t)doc["value"]);
    } else if (strcmp(cmd, "setBrightness") == 0) {
        reactor.setBrightness((uint8_t)doc["value"]);
    } else if (strcmp(cmd, "setSpeed") == 0) {
        reactor.setSpeed((uint8_t)doc["value"]);
    } else if (strcmp(cmd, "setColors") == 0) {
        RGBColor core, ring;
        if (doc["core"].is<const char*>()) {
            long num = strtol(doc["core"].as<String>().substring(1).c_str(), NULL, 16);
            core = {(uint8_t)(num >> 16), (uint8_t)(num >> 8), (uint8_t)num};
            reactor.setCoreColor(core.r, core.g, core.b);
        }
        if (doc["ring"].is<const char*>()) {
            long num = strtol(doc["ring"].as<String>().substring(1).c_str(), NULL, 16);
            ring = {(uint8_t)(num >> 16), (uint8_t)(num >> 8), (uint8_t)num};
            reactor.setRingColor(ring.r, ring.g, ring.b);
        }
    } else if (strcmp(cmd, "setPower") == 0) {
        reactor.setPower(doc["value"].as<bool>());
    } else if (strcmp(cmd, "unibeam") == 0) {
        reactor.triggerUnibeam(1800);
    } else if (strcmp(cmd, "preset") == 0) {
        reactor.applyPreset((uint8_t)doc["value"]);
    }

    broadcastState(true);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) broadcastState(true);
    else if (type == WS_EVT_DATA) handleWebSocketMessage(arg, data, len);
}

void setup() {
    Serial.begin(115200);
    reactor.begin();
    reactor.triggerUnibeam(900);

    if (!LittleFS.begin(true)) {
        Serial.println("[ERR] LittleFS Failed");
    } else if (!LittleFS.exists("/index.html")) {
        Serial.println("[ERR] Web UI missing from LittleFS; run 'pio run --target uploadfs'");
    }

    apSSID = buildDynamicSSID();
    WiFi.mode(WIFI_AP);
    IPAddress ip(192, 168, 4, 1);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID.c_str(), AP_PASS, AP_CHANNEL, 0, AP_MAX_CONN);
    WiFi.setTxPower(WIFI_POWER_15dBm);

    ArduinoOTA.setHostname("arc-reactor");
    ArduinoOTA.setPassword(AP_PASS);
    ArduinoOTA.onStart([]() {
        Serial.println("[OTA] Update started");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update complete; restarting");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Update failed (error %u)\n", error);
    });
    ArduinoOTA.begin();

    dnsServer.start(53, "*", ip);
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.onNotFound([](AsyncWebServerRequest *req) {
        if (req->url() == "/") {
            req->send(404, "text/plain", "Web UI not found. Upload the LittleFS assets with 'pio run --target uploadfs'.");
            return;
        }
        req->redirect("http://192.168.4.1/");
    });
    server.begin();
}

void loop() {
    ArduinoOTA.handle();
    dnsServer.processNextRequest();
    ws.cleanupClients();
    reactor.update();

    uint32_t now = millis();
    if (now - lastTelemetryMs >= TELEMETRY_RATE_MS) {
        lastTelemetryMs = now;
        broadcastState();
    }
}

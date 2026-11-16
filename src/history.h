#ifndef HISTORY_H
#define HISTORY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <esp_timer.h>
#include <math.h>

namespace History {
constexpr uint32_t intervalMs = 5UL * 60UL * 1000UL;
constexpr uint32_t durationSeconds = 12UL * 60UL * 60UL;
constexpr size_t maxSamples = 145;
const char *const path = "/history.json";
const char *const temporaryPath = "/history.tmp";

JsonDocument samples;
SemaphoreHandle_t mutex = nullptr;
bool ready = false;
bool attempted = false;
uint32_t lastAttempt = 0;

uint32_t uptimeSeconds() {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000000ULL);
}

bool save(const JsonDocument &document) {
  File file = LittleFS.open(temporaryPath, FILE_WRITE);
  if (!file) {
    Serial.println("Verlauf: temporaere Datei konnte nicht geoeffnet werden");
    return false;
  }
  const size_t expected = measureJson(document);
  const size_t written = serializeJson(document, file);
  file.close();
  if (written != expected) {
    Serial.println("Verlauf: Datei konnte nicht vollstaendig geschrieben werden");
    return false;
  }
  if (!LittleFS.rename(temporaryPath, path)) {
    Serial.println("Verlauf: Datei konnte nicht ersetzt werden");
    return false;
  }
  return true;
}

void begin() {
  mutex = xSemaphoreCreateMutex();
  if (!mutex) {
    Serial.println("Verlauf: Mutex konnte nicht erstellt werden");
    return;
  }
  if ((LittleFS.exists(path) && !LittleFS.remove(path)) ||
      (LittleFS.exists(temporaryPath) && !LittleFS.remove(temporaryPath))) {
    Serial.println("Verlauf: alte Daten konnten nicht geloescht werden");
    return;
  }
  samples.to<JsonArray>();
  ready = save(samples);
}

void update(bool sensorReady, float temperature, float pressurePa) {
  if (!ready || !sensorReady) return;

  const uint32_t nowMs = millis();
  if (attempted && nowMs - lastAttempt < intervalMs) return;
  attempted = true;
  lastAttempt = nowMs;

  if (!isfinite(temperature) || !isfinite(pressurePa) || pressurePa <= 0) {
    Serial.println("Verlauf: ungueltige Sensorwerte, Messung ausgelassen");
    return;
  }

  const uint32_t now = uptimeSeconds();
  xSemaphoreTake(mutex, portMAX_DELAY);
  JsonDocument next;
  next.set(samples);
  JsonArray entries = next.as<JsonArray>();
  while (entries.size() > 0 &&
         (entries.size() >= maxSamples ||
          now - (entries[0]["seconds"] | 0UL) > durationSeconds)) {
    entries.remove(0);
  }
  JsonObject entry = entries.add<JsonObject>();
  entry["seconds"] = now;
  entry["temperature"] = temperature;
  entry["pressure"] = pressurePa / 100.0f;
  if (next.overflowed()) {
    Serial.println("Verlauf: nicht genug Speicher fuer Messwerte");
  } else if (save(next)) {
    samples.set(next);
    if (samples.overflowed()) {
      ready = false;
      Serial.println("Verlauf: nicht genug Speicher fuer Messwerte");
    }
  }
  xSemaphoreGive(mutex);
}

void registerRoutes(AsyncWebServer &server) {
  server.on("/verlauf.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/verlauf.html", "text/html");
  });
  server.on("/history.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!mutex) {
      request->send(503, "text/plain", "Verlauf nicht verfuegbar");
      return;
    }
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (!ready) {
      xSemaphoreGive(mutex);
      request->send(503, "text/plain", "Verlauf nicht verfuegbar");
      return;
    }
    String serializedSamples;
    const size_t expected = measureJson(samples);
    serializeJson(samples, serializedSamples);
    String body;
    const bool complete = serializedSamples.length() == expected &&
                          body.reserve(expected + 32) &&
                          body.concat("{\"now\":") &&
                          body.concat(uptimeSeconds()) &&
                          body.concat(",\"samples\":") &&
                          body.concat(serializedSamples) &&
                          body.concat("}");
    xSemaphoreGive(mutex);
    if (!complete) {
      Serial.println("Verlauf: Antwort konnte nicht erstellt werden");
      request->send(500, "text/plain", "Verlauf konnte nicht erstellt werden");
      return;
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", body);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
  });
}
}

#endif

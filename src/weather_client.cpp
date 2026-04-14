#include "weather_client.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Weather runs on-demand/infrequently, so give the remote API a little breathing room.
static constexpr unsigned long HTTP_TIMEOUT_MS = 4000;

static String normalizeCityName(const String& city) {
    String value = city;
    value.trim();
    if (value == "深圳" || value.equalsIgnoreCase("shenzhen")) return "Shenzhen";
    if (value == "北京" || value.equalsIgnoreCase("beijing")) return "Beijing";
    if (value == "上海" || value.equalsIgnoreCase("shanghai")) return "Shanghai";
    if (value == "广州" || value.equalsIgnoreCase("guangzhou")) return "Guangzhou";
    if (value == "杭州" || value.equalsIgnoreCase("hangzhou")) return "Hangzhou";
    if (value == "成都" || value.equalsIgnoreCase("chengdu")) return "Chengdu";
    if (value == "武汉" || value.equalsIgnoreCase("wuhan")) return "Wuhan";
    return value;
}

static bool isUrlSafe(char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~';
}

static String urlEncode(const String& value) {
    String encoded;
    static const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < value.length(); i++) {
        uint8_t c = (uint8_t)value[i];
        if (isUrlSafe((char)c)) {
            encoded += (char)c;
        } else if (c == ' ') {
            encoded += "%20";
        } else {
            encoded += '%';
            encoded += hex[(c >> 4) & 0x0F];
            encoded += hex[c & 0x0F];
        }
    }
    return encoded;
}

static bool connectWeatherHost(WiFiClient& client, const char* host) {
    client.setTimeout(HTTP_TIMEOUT_MS);

    IPAddress resolvedIp;
    if (!WiFi.hostByName(host, resolvedIp)) {
        Serial.printf("[WEATHER] DNS failed: %s\n", host);
        return false;
    }

    Serial.printf("[WEATHER] DNS %s -> %s\n", host, resolvedIp.toString().c_str());
    if (client.connect(host, 80)) return true;

    Serial.printf("[WEATHER] Host connection failed: %s\n", host);
    client.stop();
    delay(20);
    client.setTimeout(HTTP_TIMEOUT_MS);
    if (client.connect(resolvedIp, 80)) return true;

    Serial.printf("[WEATHER] IP connection failed: %s\n", resolvedIp.toString().c_str());
    return false;
}

// Skip HTTP response headers (zero heap allocation).
// Detects \r\n\r\n (standard) or \n\n (lenient) as end-of-headers.
// Also scans for "Transfer-Encoding: chunked" and sets *chunked flag.
static bool skipHeaders(Client& client, unsigned long deadline, bool* chunked) {
    *chunked = false;
    // State: 0=in content, 1=saw \r, 2=saw \n (line ended), 3=saw \r after line end
    int state = 0;
    // Ring buffer to detect "chunked" keyword
    char ring[7] = {};
    int ringPos = 0;
    while ((client.connected() || client.available()) && millis() < deadline) {
        if (!client.available()) { delay(1); continue; }
        char c = client.read();

        // Case-insensitive scan for "chunked"
        ring[ringPos] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
        ringPos = (ringPos + 1) % 7;
        static const char target[] = "chunked";
        bool match = true;
        for (int i = 0; i < 7 && match; i++) {
            if (ring[(ringPos + i) % 7] != target[i]) match = false;
        }
        if (match) *chunked = true;

        switch (state) {
            case 0:
                if (c == '\r') state = 1;
                else if (c == '\n') state = 2;
                break;
            case 1:
                if (c == '\n') state = 2;
                else if (c == '\r') state = 1;
                else state = 0;
                break;
            case 2:
                if (c == '\n') return true;
                if (c == '\r') state = 3;
                else state = 0;
                break;
            case 3:
                if (c == '\n') return true;
                else state = 0;
                break;
        }
    }
    return false;
}

// Read HTTP body into buffer with deadline.
// Handles chunked transfer-encoding or reads until connection closes.
static int readBody(Client& client, char* buf, int bufSize, unsigned long deadline, bool chunked) {
    int len = 0;
    if (chunked) {
        while (len < bufSize - 1 && millis() < deadline) {
            // Read chunk size line
            char sizeBuf[16];
            int sizePos = 0;
            while (sizePos < (int)sizeof(sizeBuf) - 1 && millis() < deadline) {
                if (!client.available()) {
                    if (!client.connected()) goto done;
                    delay(1); continue;
                }
                char c = client.read();
                if (c == '\n') break;
                if (c != '\r') sizeBuf[sizePos++] = c;
            }
            sizeBuf[sizePos] = '\0';
            int chunkSize = (int)strtol(sizeBuf, nullptr, 16);
            if (chunkSize <= 0) break;

            for (int i = 0; i < chunkSize && len < bufSize - 1 && millis() < deadline; i++) {
                while (!client.available() && client.connected() && millis() < deadline) delay(1);
                if (client.available()) buf[len++] = client.read();
            }
            // Skip trailing \r\n
            for (int i = 0; i < 2 && millis() < deadline; i++) {
                while (!client.available() && client.connected() && millis() < deadline) delay(1);
                if (client.available()) client.read();
            }
        }
    } else {
        while (len < bufSize - 1 && millis() < deadline) {
            if (client.available()) {
                buf[len++] = client.read();
            } else if (!client.connected()) {
                break;
            } else {
                delay(1);
            }
        }
    }
done:
    buf[len] = '\0';
    return len;
}

void WeatherClient::begin(const String& city) {
    cityName = normalizeCityName(city);
    hasCoords = false;
    data.valid = false;
    lastUpdate = 0;
    lastResolveAttempt = 0;
    Serial.printf("[WEATHER] Init deferred, city: %s\n", cityName.c_str());
}

void WeatherClient::update() {
    if (WiFi.status() != WL_CONNECTED) return;

    unsigned long now = millis();
    if (!hasCoords) {
        if (cityName.length() == 0) return;
        if (lastResolveAttempt != 0 && now - lastResolveAttempt < RESOLVE_RETRY_INTERVAL) return;
        lastResolveAttempt = now;
        if (!resolveCity(cityName)) return;
        // Resolve succeeded; fetch immediately once.
        fetchWeather();
        return;
    }

    if (now - lastUpdate >= UPDATE_INTERVAL) {
        fetchWeather();
    }
}

bool WeatherClient::resolveCity(const String& city) {
    if (city.length() == 0) {
        Serial.println("[WEATHER] No city configured");
        return false;
    }

    WiFiClient client;

    if (!connectWeatherHost(client, "geocoding-api.open-meteo.com")) {
        Serial.println("[WEATHER] Geocoding connection failed");
        return false;
    }

    String encodedCity = urlEncode(normalizeCityName(city));
    char path[192];
    int written = snprintf(path, sizeof(path), "/v1/search?name=%s&count=1&language=en", encodedCity.c_str());
    if (written >= (int)sizeof(path)) {
        Serial.println("[WEATHER] City name too long");
        client.stop();
        return false;
    }

    client.printf("GET %s HTTP/1.1\r\n", path);
    client.println("Host: geocoding-api.open-meteo.com");
    client.println("User-Agent: Catputer/1.0");
    client.println("Accept: application/json");
    client.println("Connection: close");
    client.println();

    unsigned long deadline = millis() + HTTP_TIMEOUT_MS;

    bool chunked = false;
    if (!skipHeaders(client, deadline, &chunked)) {
        Serial.println("[WEATHER] Geocoding timeout (headers)");
        client.stop();
        return false;
    }

    char body[512];
    int bodyLen = readBody(client, body, sizeof(body), deadline, chunked);
    client.stop();

    if (bodyLen == 0) {
        Serial.println("[WEATHER] Geocoding empty response");
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body, bodyLen);
    if (err) {
        Serial.printf("[WEATHER] Geocoding JSON error: %s\n", err.c_str());
        return false;
    }

    JsonArray results = doc["results"];
    if (results.isNull() || results.size() == 0) {
        Serial.printf("[WEATHER] City not found: %s\n", city.c_str());
        return false;
    }

    lat = results[0]["latitude"] | 0.0f;
    lon = results[0]["longitude"] | 0.0f;
    hasCoords = true;

    Serial.printf("[WEATHER] Geocoded %s -> %.2f, %.2f\n", city.c_str(), lat, lon);
    return true;
}

bool WeatherClient::fetchWeather() {
    WiFiClient client;

    if (!connectWeatherHost(client, "api.open-meteo.com")) {
        Serial.println("[WEATHER] Weather API connection failed");
        return false;
    }

    char path[192];
    snprintf(path, sizeof(path),
        "/v1/forecast?latitude=%.2f&longitude=%.2f"
        "&current=temperature_2m,relative_humidity_2m,weather_code,is_day&timezone=auto",
        lat, lon);

    client.printf("GET %s HTTP/1.1\r\n", path);
    client.println("Host: api.open-meteo.com");
    client.println("User-Agent: Catputer/1.0");
    client.println("Accept: application/json");
    client.println("Connection: close");
    client.println();

    unsigned long deadline = millis() + HTTP_TIMEOUT_MS;

    bool chunked = false;
    if (!skipHeaders(client, deadline, &chunked)) {
        Serial.println("[WEATHER] Weather timeout (headers)");
        client.stop();
        return false;
    }

    char body[512];
    int bodyLen = readBody(client, body, sizeof(body), deadline, chunked);
    client.stop();

    if (bodyLen == 0) {
        Serial.println("[WEATHER] Weather empty response");
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body, bodyLen);
    if (err) {
        Serial.printf("[WEATHER] Weather JSON error: %s\n", err.c_str());
        return false;
    }

    JsonObject current = doc["current"];
    if (current.isNull()) {
        Serial.println("[WEATHER] No 'current' field in response");
        return false;
    }

    data.temperature = current["temperature_2m"] | 0.0f;
    data.humidity = current["relative_humidity_2m"] | 0;
    int code = current["weather_code"] | 0;
    data.isDay = (current["is_day"] | 1) != 0;
    data.type = codeToType(code);
    data.valid = true;
    lastUpdate = millis();

    Serial.printf("[WEATHER] %.1f C, %d%% RH, code=%d, type=%d, day=%d\n",
                  data.temperature, data.humidity, code, (int)data.type, data.isDay);
    return true;
}

WeatherType WeatherClient::codeToType(int code) {
    if (code == 0)                         return WeatherType::CLEAR;
    if (code >= 1 && code <= 2)            return WeatherType::PARTLY_CLOUDY;
    if (code == 3)                         return WeatherType::OVERCAST;
    if (code == 45 || code == 48)          return WeatherType::FOG;
    if (code >= 51 && code <= 57)          return WeatherType::DRIZZLE;
    if ((code >= 61 && code <= 67) ||
        (code >= 80 && code <= 82))        return WeatherType::RAIN;
    if ((code >= 71 && code <= 77) ||
        (code >= 85 && code <= 86))        return WeatherType::SNOW;
    if (code >= 95 && code <= 99)          return WeatherType::THUNDER;
    return WeatherType::UNKNOWN;
}

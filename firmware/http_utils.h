#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "secrets.h"
#include "SIM800SmsManager.h"

// Externo: manejador de SMS definido en tu .ino
extern SIM800SmsManager smsMgr;

// =========================
//   Configuración general
// =========================
#ifndef API_URL
  #define API_URL "http://192.168.3.137:5000/api/data"
#endif

#ifndef MIRROR_URL
  #define MIRROR_URL ""
#endif

#ifndef HTTP_POST_RETRIES
  #define HTTP_POST_RETRIES 3
#endif

#ifndef HTTP_POST_TIMEOUT_MS
  #define HTTP_POST_TIMEOUT_MS 15000
#endif

/*#ifndef HTTP_ALERT_SMS
  #define HTTP_ALERT_SMS(msg) smsMgr.sendSmsReliable(TEST_PHONE, msg)
#endif*/

// =========================
//   Utilidades internas
// =========================
inline void httpAddCommonHeaders(HTTPClient& http, bool isPrimary) {
  (void)isPrimary;
  http.addHeader("Content-Type", "application/json");

  #ifdef API_KEY
    http.addHeader("x-api-key", API_KEY);
  #endif
  #ifdef BEARER_TOKEN
    http.addHeader("Authorization", String("Bearer ") + BEARER_TOKEN);
  #endif
}

#if USE_HTTPS == 1
  #include <WiFiClientSecure.h>
  inline bool httpPostOnce(WiFiClientSecure& client, const char* url, const String& json, int& codeOut, String* bodyOut = nullptr) {
    HTTPClient http;
    if (!http.begin(client, url)) {
      Serial.printf("[HTTP] begin() fallo para URL: %s\n", url);
      codeOut = -1001;
      return false;
    }
    #if HTTPS_INSECURE
      client.setInsecure();
    #endif
    http.setTimeout(HTTP_POST_TIMEOUT_MS);
    httpAddCommonHeaders(http, true);

    int code = http.POST(json);
    if (code > 0) {
      Serial.printf("[HTTP] POST %s -> code: %d\n", url, code);
      if (bodyOut) *bodyOut = http.getString();
    } else {
      Serial.printf("[HTTP] POST %s -> error code: %d\n", url, code);
    }
    http.end();
    codeOut = code;
    return (code >= 200 && code < 300);
  }
#else
  #include <WiFiClient.h>
  inline bool httpPostOnce(WiFiClient& client, const char* url, const String& json, int& codeOut, String* bodyOut = nullptr) {
    HTTPClient http;
    if (!http.begin(client, url)) {
      Serial.printf("[HTTP] begin() fallo para URL: %s\n", url);
      codeOut = -1001;
      return false;
    }
    http.setTimeout(HTTP_POST_TIMEOUT_MS);
    httpAddCommonHeaders(http, true);

    int code = http.POST(json);
    if (code > 0) {
      Serial.printf("[HTTP] POST %s -> code: %d\n", url, code);
      if (bodyOut) *bodyOut = http.getString();
    } else {
      Serial.printf("[HTTP] POST %s -> error code: %d\n", url, code);
    }
    http.end();
    codeOut = code;
    return (code >= 200 && code < 300);
  }
#endif

// =========================
//   API pública
// =========================
inline bool enviarJsonAPI(const String& json) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi no conectado, no se puede enviar JSON.");
    //HTTP_ALERT_SMS("Error: sin WiFi; no se envio JSON");
    return false;
  }

  int code = -1000;
  bool ok = false;

  #if USE_HTTPS == 1
    WiFiClientSecure client;
    #if HTTPS_INSECURE
      client.setInsecure();
    #endif
  #else
    WiFiClient client;
  #endif

  for (int i = 0; i < HTTP_POST_RETRIES; ++i) {
    ok = httpPostOnce(client, API_URL, json, code, nullptr);
    if (ok || (code > 0 && code < 500)) break;
    delay(400 + i * 400);
  }

  if (ok) return true;

  if (String(MIRROR_URL).length() > 0) {
    Serial.println("[HTTP] Intentando espejo (mirror) para observabilidad...");
    String mirrorBody;
    int mirrorCode = -1000;
    bool mirrorOk = httpPostOnce(client, MIRROR_URL, json, mirrorCode, &mirrorBody);
    Serial.printf("[HTTP] Mirror code: %d\n", mirrorCode);
    if (mirrorOk) {
      Serial.println("[HTTP] Mirror body:");
      Serial.println(mirrorBody);
    } else {
      Serial.println("[HTTP] Mirror fallo.");
    }
  }

  //HTTP_ALERT_SMS(String("Error API (") + code + ")");
  return false;
}

#endif // HTTP_UTILS_H

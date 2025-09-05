#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <WiFi.h>
#include "config.h"

#ifndef WIFI_CONNECT_TIMEOUT_MS
  #define WIFI_CONNECT_TIMEOUT_MS 30000  // 30s
#endif

static void wifiPrintScan() {
  Serial.println("[WiFi] Escaneando redes 2.4GHz...");
  int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  if (n <= 0) {
    Serial.println("[WiFi] No se encontraron redes.");
    return;
  }
  for (int i = 0; i < n; i++) {
    Serial.printf("  - SSID: %s  RSSI: %d  %s  ch:%d\n",
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                  (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "OPEN" : "LOCK"),
                  WiFi.channel(i));
  }
}

inline void conectarWiFi() {
  Serial.println("[WiFi] Iniciando modo estación...");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);            // evita ahorro de energía que a veces molesta
  WiFi.setAutoReconnect(true);
  WiFi.disconnect(true, true);     // limpio antes de conectar
  delay(200);

#ifdef WIFI_HOSTNAME
  WiFi.setHostname(WIFI_HOSTNAME);
#else
  WiFi.setHostname("tiveg-concentrador");
#endif

  wifiPrintScan();                 // imprime redes visibles (confirma que ves tu SSID)

  Serial.printf("[WiFi] Conectando a SSID: '%s'\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long t0 = millis();
  wl_status_t st;
  do {
    delay(300);
    st = WiFi.status();
    Serial.printf(".");
    if ((millis() - t0) > WIFI_CONNECT_TIMEOUT_MS) break;
  } while (st != WL_CONNECTED);

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] ¡Conectado!");
    Serial.print("       IP: "); Serial.println(WiFi.localIP());
    Serial.print("       RSSI: "); Serial.println(WiFi.RSSI());
  } else {
    Serial.println("[WiFi] ERROR: No se pudo conectar.");
    Serial.println("        Revisa SSID/Password, 2.4GHz y canal del AP.");
  }
}

inline void checkWiFiLoop(unsigned long delayReconnect = 10000) {
  static unsigned long last = 0;
  if (millis() - last >= delayReconnect) {
    last = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Conexión perdida. Reintentando...");
      conectarWiFi();
    }
  }
}

#endif

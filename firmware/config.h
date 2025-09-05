#ifndef CONFIG_H
#define CONFIG_H

// ================================
//   Selección de tarjeta
// ================================
// 0 = TTGO T-Call (ESP32 + SIM800L)
// 1 = DualMCU ONE (ESP32 + RP2040)
#define USE_BOARD 0

// ================================
//   Configuración WiFi
// ================================
// Se toma de secrets.h (WIFI_SSID, WIFI_PASSWORD)

// ================================
//   Configuración HTTP/HTTPS
// ================================
// 0 = usar HTTP plano
// 1 = usar HTTPS (WiFiClientSecure)
#define USE_HTTPS 1

// Si USE_HTTPS = 1:
//    0 = validar certificado (producción)
//    1 = ignorar certificado (solo pruebas)
#define HTTPS_INSECURE 1

// ================================
//   Configuración SIM / Modem
// ================================
#define SIM_BAUD 9600

#if USE_BOARD == 0
  // ---- Pines TTGO T-Call ----
  #define MODEM_RST       5
  #define MODEM_PWKEY     4
  #define MODEM_POWER_ON 23
  #define MODEM_TX       27
  #define MODEM_RX       26

#elif USE_BOARD == 1
  // ---- Pines DualMCU ONE ----
  #define MODEM_RST       21
  #define MODEM_PWKEY     22
  #define MODEM_POWER_ON  19
  #define MODEM_TX        17
  #define MODEM_RX        16
#endif

// LED de estado
#define STATUS_LED 25

#endif // CONFIG_H

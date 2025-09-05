// ===== TIVEG Concentrador – versión modularizada con sanitizado =====
// TTGO T-Call / DualMCU ONE + SIM800L
// Recepción SMS en modo PDU + UDH → JSON → API

#include "secrets.h"
#include "config.h"
#include "wifi_config.h"
#include "http_utils.h"

#include "SIM800SmsManager.h"
#include "SmsPduParser.h"
#include "SmsConcatManager.h"
#include "JsonQueue.h"

HardwareSerial MODEM(2);
SIM800SmsManager smsMgr(MODEM);
SmsConcatManager concatMgr;
JsonQueue queueJson;

// Buffer temporal para URCs
static String modemLine;

// === Sanitizar JSON recibido ===
static String sanitizeJson(const String& raw) {
  String out;
  out.reserve(raw.length());
  for (size_t i = 0; i < raw.length(); i++) {
    char c = raw[i];
    if ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t') {
      out += c;
    }
  }
  out.trim();
  return out;
}

static void powerOnBoard() {
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);

  digitalWrite(MODEM_POWER_ON, HIGH);
  delay(100);
  digitalWrite(MODEM_RST, HIGH);
  delay(100);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(1200);
  digitalWrite(MODEM_PWKEY, HIGH);
  delay(3500);
}

// === Procesa línea URC ===
static void processModemLine(const String& line) {
  if (line.startsWith("+CMTI")) {
    int c = line.lastIndexOf(',');
    if (c > 0) {
      int idx = line.substring(c + 1).toInt();
      String pdu;
      if (SmsPduParser::readPduAtIndex(MODEM, idx, pdu)) {
        SmsPduInfo info;
        if (SmsPduParser::parseSmsDeliverPdu(pdu, info)) {
          String assembled = concatMgr.addPart(info, idx);
          if (!assembled.isEmpty()) {
            assembled = sanitizeJson(assembled);
            if (assembled.startsWith("(") && assembled.endsWith(")")) {
              assembled = "{" + assembled.substring(1, assembled.length() - 1) + "}";
            }
            if (!queueJson.push(assembled)) enviarJsonAPI(assembled);

            // 🔹 Borrar TODAS las partes del mensaje ensamblado
            for (int i = 0; i < MAX_PARTS; i++) {
              int smsIdx = concatMgr.getPartIndex(info, i);
              if (smsIdx > 0) {
                smsMgr.at("AT+CMGD=" + String(smsIdx), 1000, true);
                Serial.printf("[CMTI] SMS idx=%d borrado de ME\n", smsIdx);
              }
            }
            concatMgr.reset(info);  // limpia entrada interna
          }
        }
      }
    }
  }
}

// === Sweep de seguridad ===
static void sweepMessages(int status) {
  // status: 0 = UNREAD, 1 = READ
  String resp = smsMgr.at("AT+CMGL=" + String(status), 6000, true);
  int pos = 0;
  while (true) {
    int idxPos = resp.indexOf("+CMGL:", pos);
    if (idxPos < 0) break;

    int comma = resp.indexOf(',', idxPos);
    if (comma < 0) break;

    int idx = resp.substring(idxPos + 6, comma).toInt();
    Serial.printf("[SWEEP] Encontrado SMS idx=%d (status=%d)\n", idx, status);

    String pdu;
    if (SmsPduParser::readPduAtIndex(MODEM, idx, pdu)) {
      SmsPduInfo info;
      if (SmsPduParser::parseSmsDeliverPdu(pdu, info)) {
        String assembled = concatMgr.addPart(info, idx);
        if (!assembled.isEmpty()) {
          assembled = sanitizeJson(assembled);
          if (assembled.startsWith("(") && assembled.endsWith(")")) {
            assembled = "{" + assembled.substring(1, assembled.length() - 1) + "}";
          }
          if (!queueJson.push(assembled)) enviarJsonAPI(assembled);

          // 🔹 Borrar TODAS las partes del mensaje ensamblado
          for (int i = 0; i < MAX_PARTS; i++) {
            int smsIdx = concatMgr.getPartIndex(info, i);
            if (smsIdx > 0) {
              smsMgr.at("AT+CMGD=" + String(smsIdx), 1000, true);
              Serial.printf("[SWEEP] SMS idx=%d borrado de ME\n", smsIdx);
            }
          }
          concatMgr.reset(info);
        }
      }
    }
    pos = comma + 1;
  }
}

void setup() {
  pinMode(STATUS_LED, OUTPUT);
  Serial.begin(115200);
  Serial.println("\n=== TIVEG SMS PDU/UDH concentrador (modular) ===");

  powerOnBoard();
  MODEM.begin(SIM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);

  conectarWiFi();

  // Init SIM800
  smsMgr.at("AT");
  smsMgr.at("ATE0");
  smsMgr.at("AT+CMEE=2");
  smsMgr.at("AT+CMGF=0");  // PDU
  smsMgr.at("AT+CPMS=\"ME\",\"ME\",\"ME\"");
  smsMgr.at("AT+CNMI=2,1,0,0,0");
  smsMgr.at("AT+CSCLK=0");

  // SMS de arranque
  //smsMgr.sendSmsReliable(TEST_PHONE, "TIVEG concentrador iniciado OK");
}

bool consoleMode = false;  // OFF al inicio

void loop() {
  // --- Modo consola (###) ---
  static String serialCmd;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialCmd == "###") {
        consoleMode = !consoleMode;
        Serial.printf("\n[Modo %s]\n", consoleMode ? "CONSOLA" : "CONCENTRADOR");
      } else if (consoleMode) {
        if (serialCmd == "::Z") {
          MODEM.write((uint8_t)0x1A);
          Serial.println("[CONSOLA] Enviado Ctrl+Z (0x1A)");
        } else {
          MODEM.print(serialCmd);
          MODEM.print("\r\n");
        }
      }
      serialCmd = "";
    } else {
      serialCmd += c;
    }
  }

  if (consoleMode) {
    while (MODEM.available()) Serial.write(MODEM.read());
    return;
  }

  // --- URCs del módem ---
  while (MODEM.available()) {
    char c = MODEM.read();
    if (c == '\n') {
      modemLine.trim();
      if (modemLine.length()) processModemLine(modemLine);
      modemLine = "";
    } else if (c != '\r') modemLine += c;
  }

  // --- Envío a API ---
static uint32_t lastTry = 0;
static uint32_t retryDelay = 100;
static int retryCount = 0;   // 🔹 contar reintentos del mismo mensaje

if (!queueJson.empty() && millis() - lastTry > retryDelay) {
  lastTry = millis();
  String& j = queueJson.front();
  Serial.printf("[API] Enviando JSON (%u bytes)...\n", (unsigned)j.length());
  if (enviarJsonAPI(j)) {
    Serial.println("[API] OK");
    queueJson.pop();
    retryDelay = 100;
    retryCount = 0;
  } else {
    Serial.println("[API] Error; reintento posterior");
    retryDelay = min<uint32_t>(retryDelay * 2, 60000);
    retryCount++;

    // 🔹 si ya falló demasiadas veces, descartamos
    if (retryCount >= 1) {
      Serial.println("[API] ❌ Descartando mensaje tras 1 fallos");
      queueJson.pop();
      retryDelay = 100;
      retryCount = 0;
    }
  }
}


  // --- Sweep de seguridad cada 60s ---
  static uint32_t lastSweep = 0;
  if (millis() - lastSweep > 10000) {
    lastSweep = millis();
    Serial.println("[SWEEP] Revisando UNREAD...");
    sweepMessages(0);
    Serial.println("[SWEEP] Revisando READ...");
    sweepMessages(1);
  }

  // --- Housekeeping ---
  concatMgr.housekeeping();
  checkWiFiLoop(8000);
}

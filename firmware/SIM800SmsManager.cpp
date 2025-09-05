#include "SIM800SmsManager.h"

bool SIM800SmsManager::begin(uint32_t baud, int rxPin, int txPin) {
  if (rxPin >= 0 && txPin >= 0) modem_.begin(baud, SERIAL_8N1, rxPin, txPin);
  else modem_.begin(baud);
  return true;
}

void SIM800SmsManager::flushModem() {
  while (modem_.available()) modem_.read();
}

String SIM800SmsManager::readFor(unsigned long ms) {
  unsigned long t0 = millis();
  String r;
  while (millis() - t0 < ms) {
    while (modem_.available()) r += (char)modem_.read();
    delay(1);
  }
  return r;
}

String SIM800SmsManager::at(const String& cmd, uint16_t waitMs, bool echo) {
  flushModem();
  if (echo) { Serial.print("[AT] "); Serial.println(cmd); }
  modem_.print(cmd); modem_.print("\r\n");
  String r = readFor(waitMs);
  r.trim();
  if (echo) { Serial.println(r); Serial.println("------------------------"); }
  return r;
}

bool SIM800SmsManager::sendSmsReliable(const String& number, const String& text) {
  at("AT+CMGF=1", 1000, false);
  flushModem();

  modem_.print("AT+CMGS=\""); modem_.print(number); modem_.print("\"\r\n");
  String prompt = readFor(5000);
  if (prompt.indexOf('>') == -1) {
    Serial.println("[SMS] No prompt '>'");
    return false;
  }

  modem_.print(text);
  modem_.write((uint8_t)0x1A); // CTRL+Z

  String resp = readFor(15000);
  bool ok = (resp.indexOf("+CMGS:") != -1 && resp.indexOf("OK") != -1);
  Serial.println(ok ? "[SMS] Enviado OK" : "[SMS] Fallo envío");
  at("AT+CMGF=0");  // volver a PDU
  return ok;
}

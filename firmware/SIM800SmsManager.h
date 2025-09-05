#pragma once
#include <Arduino.h>

class SIM800SmsManager {
public:
  SIM800SmsManager(HardwareSerial& modem) : modem_(modem) {}

  bool begin(uint32_t baud = 9600, int rxPin = -1, int txPin = -1);
  String at(const String& cmd, uint16_t waitMs = 1500, bool echo = true);

  // Nuevo: envío confiable de SMS en modo texto
  bool sendSmsReliable(const String& number, const String& text);

private:
  void flushModem();
  String readFor(unsigned long ms);

  HardwareSerial& modem_;
};

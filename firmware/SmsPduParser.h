#pragma once
#include <Arduino.h>
#include <vector>

struct SmsPduInfo {
  String sender;
  bool hasConcat = false;
  uint16_t ref = 0;
  uint8_t total = 1;
  uint8_t seq = 1;
  String text;
};

class SmsPduParser {
public:
  static bool parseSmsDeliverPdu(const String& pduHex, SmsPduInfo& out);
  static bool readPduAtIndex(HardwareSerial& modem, int idx, String& outPdu);

private:
  static void hexToBytes(const String& hex, std::vector<uint8_t>& out);
  static String semiOctetToString(const uint8_t* b, int nDigits);
  static String gsm7Unpack(const uint8_t* data, int septets);
  static String ucs2beToUtf8(const uint8_t* p, int nBytes);
  static bool isHexChar(char c);
  static bool isLikelyHexLine(const String& s);
};

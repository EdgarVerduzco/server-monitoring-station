#include "SmsPduParser.h"

void SmsPduParser::hexToBytes(const String& hex, std::vector<uint8_t>& out) {
  out.clear();
  out.reserve(hex.length() / 2);
  auto v = [&](char c)->int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
  };
  for (int i = 0; i + 1 < hex.length(); i += 2)
    out.push_back((v(hex[i]) << 4) | v(hex[i + 1]));
}

String SmsPduParser::semiOctetToString(const uint8_t* b, int nDigits) {
  String s;
  s.reserve(nDigits);
  for (int i = 0; i < (nDigits + 1) / 2; i++) {
    uint8_t v = b[i];
    uint8_t lo = v & 0x0F, hi = (v >> 4) & 0x0F;
    s += char('0' + lo);
    if (s.length() < nDigits) s += char('0' + hi);
  }
  return s;
}

String SmsPduParser::gsm7Unpack(const uint8_t* data, int septets) {
  String out;
  out.reserve(septets);
  int i = 0, outCount = 0, carryBits = 0;
  uint8_t carry = 0;
  while (outCount < septets) {
    uint8_t b = data[i++];
    uint8_t c = ((b << carryBits) & 0x7F) | carry;
    out += char(c);
    outCount++;
    carry = (b >> (7 - carryBits)) & 0x7F;
    carryBits++;
    if (carryBits == 7) {
      if (outCount < septets) {
        out += char(carry);
        outCount++;
      }
      carry = 0; carryBits = 0;
    }
  }
  return out;
}

String SmsPduParser::ucs2beToUtf8(const uint8_t* p, int nBytes) {
  String out;
  for (int i = 0; i + 1 < nBytes; i += 2) {
    uint16_t u = (p[i] << 8) | p[i+1];
    if (u < 0x80) out += char(u);
    else if (u < 0x800) {
      out += char(0xC0 | (u >> 6));
      out += char(0x80 | (u & 0x3F));
    } else {
      out += char(0xE0 | (u >> 12));
      out += char(0x80 | ((u >> 6) & 0x3F));
      out += char(0x80 | (u & 0x3F));
    }
  }
  return out;
}

bool SmsPduParser::parseSmsDeliverPdu(const String& pduHex, SmsPduInfo& out) {
  std::vector<uint8_t> p;
  hexToBytes(pduHex, p);
  if (p.size() < 14) return false;

  int i = 0;
  uint8_t smscLen = p[i++];
  i += smscLen;

  uint8_t firstOctet = p[i++];
  bool udhi = (firstOctet & 0x40) != 0;

  uint8_t oaLenDigits = p[i++];
  uint8_t oaToA = p[i++];
  int oaBytes = (oaLenDigits + 1) / 2;
  String oa = semiOctetToString(&p[i], oaLenDigits);
  i += oaBytes;
  if ((oaToA & 0xF0) == 0x90 && oa[0] != '+') oa = "+" + oa;
  out.sender = oa;

  i++; // pid
  uint8_t dcs = p[i++];
  i += 7; // SCTS

  uint8_t udl = p[i++];
  const uint8_t* ud = &p[i];

  uint8_t udhl = 0;
  int userDataOffset = 0;
  if (udhi) {
    udhl = ud[0];
    const uint8_t* udh = &ud[1];
    int pos = 0;
    while (pos < udhl) {
      uint8_t iei = udh[pos++];
      uint8_t ielen = udh[pos++];
      if (iei == 0x00 && ielen == 3) {
        out.hasConcat = true;
        out.ref = udh[pos];
        out.total = udh[pos+1];
        out.seq = udh[pos+2];
      }
      pos += ielen;
    }
    userDataOffset = 1 + udhl;
  }

  if (dcs == 0x08) {
    int take = udl - userDataOffset;
    out.text = ucs2beToUtf8(ud + userDataOffset, take);
  } else {
    int udhSeptets = (userDataOffset * 8 + 6) / 7;
    String all = gsm7Unpack(ud, udl);
    out.text = all.substring(udhSeptets);
  }
  return true;
}

bool SmsPduParser::isHexChar(char c) {
  return (c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f');
}
bool SmsPduParser::isLikelyHexLine(const String& s) {
  if (s.length() < 20) return false;
  for (size_t i=0;i<s.length();i++){
    char c=s[i];
    if (c==' '||c=='\t'||c=='\r'||c=='\n') continue;
    if (!isHexChar(c)) return false;
  }
  return true;
}

bool SmsPduParser::readPduAtIndex(HardwareSerial& modem, int idx, String& outPdu) {
  while (modem.available()) modem.read();
  modem.printf("AT+CMGR=%d\r\n", idx);

  String buf; unsigned long t0=millis();
  while (millis()-t0<6000) {
    while (modem.available()) buf+=(char)modem.read();
    if (buf.indexOf("\nOK")!=-1||buf.indexOf("\nERROR")!=-1) break;
    delay(5);
  }

  outPdu="";
  int start=0;
  while(start<buf.length()) {
    int eol=buf.indexOf('\n',start);
    if(eol<0) eol=buf.length();
    String line=buf.substring(start,eol); line.trim();
    if(line.length()>0 && !line.startsWith("AT+CMGR") && !line.startsWith("+CMGR:") && line!="OK" && line!="ERROR") {
      if(isLikelyHexLine(line)) {
        String hex="";
        for(char c:line) if(isHexChar(c)) hex+=c;
        outPdu=hex; break;
      }
    }
    start=eol+1;
  }
  return outPdu.length()>0;
}

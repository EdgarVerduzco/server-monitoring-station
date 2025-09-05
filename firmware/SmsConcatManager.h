#ifndef SMS_CONCAT_MANAGER_H
#define SMS_CONCAT_MANAGER_H

#include "SmsPduParser.h"

#define MAX_CONCATS 10
#define MAX_PARTS 10
#define CONCAT_TIMEOUT_MS 180000

struct ConcatSlot {
  bool used = false;
  SmsPduInfo base;
  String parts[MAX_PARTS];
  int indices[MAX_PARTS];   // 🔹 índices reales en la SIM
  uint8_t received = 0;
  uint32_t lastUpdate = 0;
};

class SmsConcatManager {
public:
  SmsConcatManager();
  String addPart(const SmsPduInfo& info, int idx);
  void housekeeping();

  // 🔹 NUEVOS MÉTODOS
  int getPartIndex(const SmsPduInfo& info, int part);
  void reset(const SmsPduInfo& info);

private:
  ConcatSlot slots[MAX_CONCATS];
  int findSlot(const SmsPduInfo& info);
  int allocSlot(const SmsPduInfo& info);
};

#endif

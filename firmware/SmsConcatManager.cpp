#include "SmsConcatManager.h"
#include <Arduino.h>

SmsConcatManager::SmsConcatManager() {
  // limpia todos los slots
  for (int i = 0; i < MAX_CONCATS; i++) {
    slots[i] = ConcatSlot();
  }
}

String SmsConcatManager::addPart(const SmsPduInfo& info, int idx) {
  int slot = findSlot(info);
  if (slot < 0) slot = allocSlot(info);

  if (slot < 0) return "";

  if (info.seq >= 1 && info.seq <= slots[slot].base.total) {
    slots[slot].parts[info.seq - 1] = info.text;
    slots[slot].indices[info.seq - 1] = idx;   // guarda índice real en SIM
    slots[slot].received++;
    slots[slot].lastUpdate = millis();
  }

  // ¿completo?
  bool complete = true;
  for (uint8_t i = 0; i < slots[slot].base.total; i++) {
    if (slots[slot].parts[i].isEmpty()) {
      complete = false;
      break;
    }
  }

  if (complete) {
    String final;
    for (uint8_t i = 0; i < slots[slot].base.total; i++) {
      final += slots[slot].parts[i];
    }
    return final;
  }

  return "";
}

void SmsConcatManager::housekeeping() {
  uint32_t now = millis();
  for (int i = 0; i < MAX_CONCATS; i++) {
    if (!slots[i].used) continue;
    if (now - slots[i].lastUpdate > CONCAT_TIMEOUT_MS) {
      slots[i] = ConcatSlot();  // limpiar
    }
  }
}

int SmsConcatManager::findSlot(const SmsPduInfo& info) {
  for (int i = 0; i < MAX_CONCATS; i++) {
    if (slots[i].used &&
        slots[i].base.sender == info.sender &&
        slots[i].base.ref == info.ref) {
      return i;
    }
  }
  return -1;
}

int SmsConcatManager::allocSlot(const SmsPduInfo& info) {
  for (int i = 0; i < MAX_CONCATS; i++) {
    if (!slots[i].used) {
      slots[i].used = true;
      slots[i].base = info;
      slots[i].received = 0;
      slots[i].lastUpdate = millis();
      for (int j = 0; j < MAX_PARTS; j++) {
        slots[i].parts[j] = "";
        slots[i].indices[j] = -1;
      }
      return i;
    }
  }
  return -1;
}

int SmsConcatManager::getPartIndex(const SmsPduInfo& info, int part) {
  int s = findSlot(info);
  if (s < 0) return -1;
  if (part < 1 || part > slots[s].base.total) return -1;
  return slots[s].indices[part - 1];
}

void SmsConcatManager::reset(const SmsPduInfo& info) {
  int s = findSlot(info);
  if (s >= 0) {
    slots[s] = ConcatSlot();  // limpiar
  }
}

#pragma once
#include <Arduino.h>

#define QUEUE_MAX 16

class JsonQueue {
public:
  bool empty() const { return count==0; }
  bool push(const String& s) {
    if(count==QUEUE_MAX) return false;
    items[tail]=s;
    tail=(tail+1)%QUEUE_MAX; count++;
    return true;
  }
  String& front() { return items[head]; }
  void pop() {
    if(count){ head=(head+1)%QUEUE_MAX; count--; }
  }
private:
  String items[QUEUE_MAX];
  int head=0, tail=0, count=0;
};

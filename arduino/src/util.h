#ifndef UTIL_H
#define UTIL_H

#include "Arduino.h"

void writeByteAsBits(byte b) {
  for (byte mask = 00000001; mask>0; mask <<= 1) {
    boolean val = b & mask;
    Serial.print(val ? 1 : 0);
  }
}

void writeBitArray(boolean* buf) {
  Serial.print("Buffer: ");
  for(int i=0; i<SEGMENTS; i++) {
    Serial.print(buf[i]);
    if(i%7 == 8) {
      Serial.print(" ");
    }
  }
  Serial.println("");
}

#endif

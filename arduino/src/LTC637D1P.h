#ifndef LTC637
#define LTC637

#include "Arduino.h"

const int SEGMENTS = 28;

const int P6  = 0;
const int P7  = 1;
const int P8  = 2;
const int P9  = 3;
const int P10 = 4;
const int P12 = 5;
const int P13 = 6;
const int P15 = 7;
const int P16 = 8;
const int P17 = 9;
const int P18 = 10;
const int P19 = 11;
const int P20 = 12;
const int P21 = 13;

const boolean SEGMENT_PHASE[] = { 
  0, 1, 1, 0, 1, 0, 1,
  1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 
  1, 1, 1, 0, 0, 0, 0
};

const int SEGMENT_PIN[] = {
  P7,  P6,  P9,  P8,  P8,  P6,  P7,
  P13, P10, P12, P12, P9,  P13, P10, 
  P15, P16, P17, P17, P18, P15, P16,
  P21, P19, P20, P20, P18, P21, P19
};

const boolean FONT[][7] = { 
  { 0, 0, 0, 0, 0, 0, 0 }, // BLANK (use /)
  { 1, 1, 1, 1, 1, 1, 0 }, // 0
  { 0, 1, 1, 0, 0, 0, 0 }, // 1
  { 1, 1, 0, 1, 1, 0, 1 }, // 2
  { 1, 1, 1, 1, 0, 0, 1 }, // 3
  { 0, 1, 1, 0, 0, 1, 1 }, // 4
  { 1, 0, 1, 1, 0, 1, 1 }, // 5
  { 1, 0, 1, 1, 1, 1, 1 }, // 6
  { 1, 1, 1, 0, 0, 0, 0 }, // 7
  { 1, 1, 1, 1, 1, 1, 1 }, // 8
  { 1, 1, 1, 1, 0, 1, 1 }, // 9
  { 0, 0, 0, 0, 0, 0, 0 }, // :
  { 0, 0, 0, 0, 0, 0, 0 }, // ;
  { 0, 0, 0, 0, 0, 0, 0 }, // <
  { 0, 0, 0, 1, 0, 0, 1 }, // =
  { 0, 0, 0, 0, 0, 0, 0 }, // >
  { 1, 1, 0, 0, 1, 0, 1 }, // ?
  { 0, 0, 0, 0, 0, 0, 0 }, // @
  { 1, 1, 1, 0, 1, 1, 1 }, // A
  { 0, 0, 1, 1, 1, 1, 1 }, // B
  { 0, 0, 0, 1, 1, 0, 1 }, // C
  { 0, 1, 1, 1, 1, 0, 1 }, // D
  { 1, 0, 0, 1, 1, 1, 1 }, // E
  { 1, 0, 0, 0, 1, 1, 1 }, // F
  { 1, 0, 1, 1, 1, 1, 0 }, // G
  { 0, 0, 1, 0, 1, 1, 1 }, // H
  { 0, 1, 1, 0, 0, 0, 0 }, // I
  { 0, 1, 1, 1, 1, 0, 0 }, // J
  { 0, 1, 1, 0, 1, 1, 1 }, // K
  { 0, 0, 0, 1, 1, 1, 0 }, // L
  { 0, 0, 0, 0, 0, 0, 0 }, // M
  { 0, 0, 1, 0, 1, 0, 1 }, // N
  { 0, 0, 1, 1, 1, 0, 1 }, // O
  { 1, 1, 0, 0, 1, 1, 1 }, // P
  { 1, 1, 1, 0, 0, 1, 1 }, // Q
  { 0, 0, 0, 0, 1, 0, 1 }, // R
  { 1, 0, 1, 1, 0, 1, 1 }, // S
  { 0, 0, 0, 1, 1, 1, 1 }, // T
  { 0, 0, 1, 1, 1, 0, 0 }, // U
  { 0, 0, 1, 1, 1, 0, 0 }, // V
  { 0, 0, 0, 0, 0, 0, 0 }, // W
  { 0, 1, 1, 0, 1, 1, 1 }, // X
  { 0, 1, 1, 1, 0, 1, 1 }, // Y
  { 1, 1, 0, 1, 1, 0, 1 }  // Z
};

const byte DOT_AM = 0;
const byte DOT_PM = 1;
const byte DOT_MID = 2;

class View {
  public:
    View(int pG1, int pG2, int pClock1, int pClock2, int pLatch1, int pLatch2, int pData1, int pData2) {
      pinClock1 = pClock1;
      pinClock2 = pClock2;
      pinLatch1 = pLatch1;
      pinLatch2 = pLatch2;
      pinData1 = pData1;
      pinData2 = pData2;
      pinG1 = pG1;
      pinG2 = pG2;
      
      for(int i=0; i<sizeof(digits); i++) {
        digits[i] = 0;
      }

      for(int i=0; i<sizeof(dots); i++) {
        dots[i] = 0;
      }
      
      init();
      reset();
    }

    void setCharAndDigit(int letter, int num) {
      setCharsAndDigit('/', letter, num);
    }

    void setCharsAndDigit(int letter1, int letter2, int num) {
      digits[0] = letter1 - '/';
      digits[1] = letter2 - '/';
      digits[2] = (num / 10) % 10 + 1;
      digits[3] = num % 10 + 1;
    }

    void setDigits(int d1, int d2) {
      digits[0] = (d1 / 10) % 10 + 1;
      digits[0] = (digits[0] == 1) ? 0 : digits[0];
      digits[1] = d1 % 10 + 1;
      digits[2] = (d2 / 10) % 10 + 1;
      digits[3] = d2 % 10 + 1;
    }

    void setChar(int digit, byte val) {
      digits[digit] = val - '/';
    }

    void write(boolean phase) {
      populateBuffer();

      byte bitsToSend[] = {0,0};
      int segIdx = 0;
      for(int seg=0; seg<28; seg++) {
        if(SEGMENT_PHASE[seg] == phase) {
          int pin = SEGMENT_PIN[seg];
          boolean state = (buf[seg] == 1);
          int section = seg / 14;
          bitWrite(bitsToSend[section], pin % 7, buf[seg]);
          segIdx++;
        }
      }
      bitWrite(bitsToSend[0], 7, dots[DOT_AM]);
      bitWrite(bitsToSend[1], 7, dots[DOT_MID]);

      // shift out data to registers
      digitalWrite(pinLatch1, LOW);
      digitalWrite(pinClock1, LOW);
      shiftOut(pinData1, pinClock1, LSBFIRST, bitsToSend[0]);
      digitalWrite(pinLatch1, HIGH);

      digitalWrite(pinLatch2, LOW);
      digitalWrite(pinClock2, LOW);
      shiftOut(pinData2, pinClock2, LSBFIRST, bitsToSend[1]);
      digitalWrite(pinLatch2, HIGH);

      // select the correct write phase
      digitalWrite(pinG1, phase ? HIGH : LOW);
      digitalWrite(pinG2, phase ? LOW : HIGH);
    }

    void setDigit(int idx, int value) {
      digits[idx] = value;
    }

    void setDotState(int idx, boolean state) {
      dots[idx] = state;
    }

  private:
    int pinLatch1;
    int pinLatch2;
    int pinClock1;
    int pinClock2;
    int pinData1;
    int pinData2;
    int pinG1;
    int pinG2;
    int digits[4];
    boolean dots[3];
    boolean buf[28];

    void init() {
      pinMode(pinG1, OUTPUT);
      pinMode(pinG2, OUTPUT);
      pinMode(pinLatch1, OUTPUT);
      pinMode(pinClock1, OUTPUT);
      pinMode(pinData1, OUTPUT);
      pinMode(pinLatch2, OUTPUT);
      pinMode(pinClock2, OUTPUT);
      pinMode(pinData2, OUTPUT);
    }

    boolean getStateByPinAndPhase(int pin, boolean phase) {
      int idx = findSegmentByPinAndPhase(pin, phase);
      return buf[idx];
    }

    int findSegmentByPinAndPhase(int pin, boolean phase) {
      for(int i=0; i<SEGMENTS; i++) {
        if(SEGMENT_PHASE[i] == phase && SEGMENT_PIN[i] == pin) {
   	  return i;
        }
      }
      return -1;
    }

    void populateBuffer() {
      for(int i=0; i<28; i++) {
        buf[i] = getSegmentState(i);
      }
    }

    boolean getSegmentState(int idx) {
      int digit = idx / 7;
      int segment = idx % 7;
      int num = digits[digit];
      return FONT[num][segment] ? true : false;
    }

    void reset() {
      for(int b=0; b<sizeof(buf); b++) {
        buf[b] = 0;
      }
    }
};

#endif

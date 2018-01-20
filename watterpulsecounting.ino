#include "LowPower.h"


#define pinSV 2
#define pinSV_irq 0 //IRQ that matches to pin 2

#define pinTV 3
#define pinTV_irq 1 //IRQ that matches to pin 3

typedef struct {
  volatile int level;
  volatile int pulseCount;
  volatile unsigned long time;
} EVENT;


EVENT irqEventTV = {0, 0, 0} , irqEventSV = {0, 0, 0};
EVENT lastEventTV = {0, 0, 0}, lastEventSV = {0, 0, 0};

void updateEvent(EVENT *event, int pin, int reset) {
  event->time = millis();
  event->level = digitalRead(pin);
  if (reset) {
    event->pulseCount = 0;
  } else {
    event->pulseCount++;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin (115200);
  pinMode(pinTV, INPUT);
  pinMode(pinSV, INPUT);

  updateEvent(&irqEventTV, pinTV, true);
  updateEvent(&irqEventSV, pinSV, true);

  memcpy (&lastEventTV, &irqEventTV, sizeof (EVENT));
  memcpy (&lastEventSV, &irqEventSV, sizeof (EVENT));

  attachInterrupt(pinTV_irq, IRQcounterTV, CHANGE);
  attachInterrupt(pinSV_irq, IRQcounterSV, CHANGE);
}

void IRQcounterTV() {
  updateEvent(&irqEventTV, pinTV, false);
}

void IRQcounterSV() {
  updateEvent(&irqEventSV, pinSV, false);
}

#define MINPULSEL 3
unsigned long lpulse = 0;

void pulse(EVENT *event ) {
  Serial.print((event->level == HIGH) ? 1 : 0);
  Serial.print(";");
  Serial.print(event->pulseCount);
  Serial.print(";");
  Serial.print(millis());
  Serial.print(";");
  Serial.println(event->time - lpulse);
  lpulse = event->time;
}

void pulse(EVENT *event, unsigned long lastTime, char type ) {
  Serial.print(type);
  Serial.print(";");
  Serial.print((event->level == HIGH) ? 1 : 0);
  Serial.print(";");
  Serial.print(event->pulseCount);
  Serial.print(";");
  Serial.print(millis());
  Serial.print(";");
  Serial.println(event->time - lastTime);
}


void vodaCheck(EVENT *lastEvent, EVENT *irqEvent, char type) {
  EVENT currentEvent;

  cli();//disable interrupts
  memcpy (&currentEvent, irqEvent, sizeof (EVENT));
  sei();//enable interrupts

  if (millis() - currentEvent.time > MINPULSEL) { //jak dlouho to je od minuleho IRQ?
    if (currentEvent.level != lastEvent->level) {
      cli();//disable interrupts
      irqEvent->pulseCount = 0;
      sei();//enable interrupts

      pulse(&currentEvent, lastEvent->time, type);
      memcpy (lastEvent, &currentEvent, sizeof (EVENT));
    }
  }  else {
    if (currentEvent.pulseCount != lastEvent ->pulseCount) {
      //pulse(&currentEvent, lastEvent->time, type + 'a' - 'A');
      pulse(&currentEvent, millis(), type + 'a' - 'A');
      lastEvent ->pulseCount = currentEvent.pulseCount;
    }

  }
}

void loop() {
  vodaCheck(&lastEventTV, &irqEventTV, 'T');
  vodaCheck(&lastEventSV, &irqEventSV, 'S');
  //delay(25);
}

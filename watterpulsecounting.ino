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

void inline updateEvent(EVENT *event) {
  event->time = millis();
  event->pulseCount++;
}

void resetEvent(EVENT *event, int pin) {
  event->time = millis();
  event->level = digitalRead(pin);  
  event->pulseCount = 0;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin (115200);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  pinMode(pinTV, INPUT);
  pinMode(pinSV, INPUT);

  resetEvent(&irqEventTV, pinTV);
  resetEvent(&irqEventSV, pinSV);

  memcpy (&lastEventTV, &irqEventTV, sizeof (EVENT));
  memcpy (&lastEventSV, &irqEventSV, sizeof (EVENT));

  //Serial.println(__TIME__);
  Serial.println("S/V;finished_state;IRQ_Cnt;laps_time;pulse_duration");
  

  attachInterrupt(pinTV_irq, IRQcounterTV, CHANGE);
  attachInterrupt(pinSV_irq, IRQcounterSV, CHANGE);
}

void IRQcounterTV() {
  updateEvent(&irqEventTV);
}

void IRQcounterSV() {
  updateEvent(&irqEventSV);
}

#define MIN_PULSE_TIME_MS 350  // delka cerneho ramecku je 700ms 
#define DEBOUNCE_TIME_MS 10
unsigned long lpulse = 0;

/*
 * returns true if puls was accepted , false when rejected
 */
int pulse(EVENT *event, char type ) {
  if (millis()-event->time<MIN_PULSE_TIME_MS) {
    //reject puls it is to short
    Serial.print('!');
    printPulse(event, type);
    return false;
  }
  printPulse(event, type);
  return true;
}

void printPulse(EVENT *event, char type ) {
  Serial.print(type);
  Serial.print(';');
  Serial.print((event->level == HIGH) ? 1 : 0);
  Serial.print(';');
  Serial.print(event->pulseCount);
  Serial.print(';');
  Serial.print(millis());
  Serial.print(';');
  Serial.println(millis()-event->time);
}


void vodaCheck(EVENT *lastEvent, EVENT *irqEvent, char type, int pin) {
  EVENT currentEvent;
  
  cli();//disable interrupts
  memcpy (&currentEvent, irqEvent, sizeof (EVENT));
  sei();//enable interrupts

  currentEvent.level=digitalRead(pin);
  
   //uz ubehla doba DEBOUNCE_TIME_MS od posledniho IRQ?
  if ((millis() - currentEvent.time > DEBOUNCE_TIME_MS) & (currentEvent.level != lastEvent->level)) { //jak dlouho to je od minuleho IRQ?
      cli();//disable interrupts
      irqEvent->pulseCount = 0;
      sei();//enable interrupts
      
      
      if (pulse(lastEvent, type)) {
        memcpy (lastEvent, &currentEvent, sizeof (EVENT));
      }
  }  else {
    /*
     * this branch is only for debugginig to see what happends during debouncing period
     */
    if (currentEvent.pulseCount != lastEvent ->pulseCount) {
      //pulse(&currentEvent, lastEvent->time, type + 'a' - 'A');
      Serial.print('*');
      printPulse(&currentEvent, type + 'a' - 'A');
      lastEvent ->pulseCount = currentEvent.pulseCount;
    }

  }
}


void test() {
   int keeplevel=lastEventTV.level;
  
  //test debouncing ie current - IRQ time <DEBOUNCE_TIME_MS
  Serial.println("Debouncing delay test 1 - last irq under debounce time and  different level");
  irqEventTV.time=millis();  //millis - irq time  has to be <  DEBOUNCE_TIME_MS
  lastEventTV.level=!lastEventTV.level;
  lastEventTV.pulseCount ++; 
  lastEventTV.time=millis()-MIN_PULSE_TIME_MS-5;
  vodaCheck(&lastEventTV, &irqEventTV, 'T', pinTV);
  Serial.println("Expected output *");

  Serial.println("Debouncing delay test 2 - last irq under debounce time and same level");
  irqEventTV.time=millis();  //millis - irq time  has to be <  DEBOUNCE_TIME_MS
  lastEventTV.level=keeplevel;
  lastEventTV.pulseCount ++; 
  lastEventTV.time=millis()-MIN_PULSE_TIME_MS-5;
  vodaCheck(&lastEventTV, &irqEventTV, 'T', pinTV);
  Serial.println("Expected output *");


  Serial.println("Debouncing delay test 3 - last irq over debounce time and different level, but pulse is short");
  irqEventTV.time=millis()-2*DEBOUNCE_TIME_MS;  //millis - irq time  has to be <  DEBOUNCE_TIME_MS
  lastEventTV.level=!keeplevel;
  lastEventTV.time=millis()-2*DEBOUNCE_TIME_MS;   // enforce 
  lastEventTV.pulseCount ++; 
  vodaCheck(&lastEventTV, &irqEventTV, 'T', pinTV);
  Serial.println("Expected output ! rejected pulse");


  Serial.println("Debouncing delay test 4 - last irq over debounce time and different level");
  irqEventTV.time=millis()-2*DEBOUNCE_TIME_MS;  //millis - irq time  has to be <  DEBOUNCE_TIME_MS
  lastEventTV.level=!keeplevel;
  lastEventTV.time=millis()-MIN_PULSE_TIME_MS-5;   // enforce 
  lastEventTV.pulseCount ++; 
  vodaCheck(&lastEventTV, &irqEventTV, 'T', pinTV);
  Serial.println("Expected output pulse");



  Serial.println("Debouncing delay test 5 - last irq over debounce time and same level");
  irqEventTV.time=millis()-2*DEBOUNCE_TIME_MS;  //debouncing timeout fullfilled
  lastEventTV.level=keeplevel;
  lastEventTV.time=millis()-MIN_PULSE_TIME_MS-5;   // enforce 
  lastEventTV.pulseCount ++; 
  vodaCheck(&lastEventTV, &irqEventTV, 'T', pinTV);
  Serial.println("Expected output *");
}
void loop() {
  //test();
  
  vodaCheck(&lastEventTV, &irqEventTV, 'T', pinTV);
  vodaCheck(&lastEventSV, &irqEventSV, 'S', pinSV);
  //delay(25);
}

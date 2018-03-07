#include "LowPower.h"

/*
 * update of program in arduino via command line 
 * avrdude-original  -v -patmega328p -carduino -P/dev/ttyUSB0 -b115200 -D -Uflash:w:watterpulsecounting.ino.hex:i 
 */

/*Change log
 * 18/02/04 - added data aggregation module sends update after 15 ms, format of sent data
 * colde_watter_pulse_count,warm_watter_pulse_count, milis since start, milis since last report
 * 32 0 15001 0
 * *s;1;89;15009;1
 * 18/02/21 - problems with emonhub data buffer contains illegal data, probably buffer got overwritten sevral times --> DEBUG disabled  //#define DEBUG_MSG
 * 18/02/27 - increased MIN_PULSE_TIME_MS from 350 to 600
 * added different time constat for hot watter and cold watter  (sensor detects surplusses on hotwatter because of wrong trimmer (10K vs 1k)
 *          - usually current max flow of watter is 5 pulses in 15sec -> SLSLS (Short, Long pulses) --> 15000=x*(3*S+2*L), where S=310 or 316, L=689 or 683 -> x = 6.499 or 5.582 --> in my flow MIN_PULSE_TIME_MS=310*5.582=1730 MS, 683 MS is for max flow
 *          
 *          -fixed bug when no interrupt -then pulse length is incorrectly calculated
 *          -increased debause time 
 */

/*
example of output
note: timestamp prefix is not part of response (added by a script) 18/01/28 11:58:33.812189; 

T   - tepla voda puls
S   - studena voda puls
*t  - irq pro teplou vodu
*s  - irq pro studenou vodu
!T  - prilis kraty puls (prechod z 0 na 1 na 0 a opacne)
!S  - prilis kraty puls (prechod z 0 na 1 na 0 a opacne)

ukazka na !
18/02/02 20:06:25.695647; T;0;84;457089251;10070
18/02/02 20:06:28.200226; *t;1;0;457089252;12
18/02/02 20:06:29.995934; *t;0;9;457089252;1
18/02/02 20:06:31.772860; !T;1;9;457089263;23
18/02/02 20:06:33.551994; !T;1;9;457089263;24
18/02/02 20:06:35.293656; !T;1;9;457089264;25
18/02/02 20:06:37.051467; !T;1;9;457089265;25
18/02/02 20:06:38.905891; *t;0;2;457089266;1
18/02/02 20:06:40.693749; *t;0;44;457089267;0
18/02/02 20:06:42.611590; *t;0;46;457089268;2


normalni vystup - s debug
S/V;finished_state;IRQ_Cnt;laps_S/V;finished_state;IRQ_Cnt;laps_time;pulse_duration
*s;0;2;10211;0
S;1;2;10232;10232
*s;0;0;10232;22
*s;1;5;14676;2
S;0;5;14697;4486
*s;1;0;14697;22
2 0 15001 15001 
S;1;0;16635;1960
*s;0;1;16636;1
*s;1;3;21105;0
S;0;3;21126;6450
*s;1;0;21126;22
*s;0;2;23071;1
*s;0;4;23072;1
S;1;4;23092;1987
*s;0;0;23092;22
3 0 30002 15001 


---
18/01/28 11:58:33.812189; 19:13:32
18/01/28 11:58:35.627988; S/V;finished_state;IRQ_Cnt;laps_time;pulse_duration
18/02/03 14:46:18.405012; *t;1;184;524348107;1
18/02/03 14:46:20.173257; *t;1;193;524348108;1
18/02/03 14:46:21.957223; *t;0;202;524348109;1
18/02/03 14:46:23.720372; *t;1;208;524348110;1
18/02/03 14:46:25.492433; T;0;208;524348120;5720935
18/02/03 14:46:28.002567; *t;1;0;524348121;12
18/02/03 14:46:29.741161; *t;1;1;524355990;1
18/02/03 14:46:31.503145; *t;0;45;524355990;1
18/02/03 14:46:33.253787; *t;0;47;524355991;1
18/02/03 14:46:35.025499; *t;0;48;524355992;1
18/02/03 14:46:36.858838; *t;0;55;524355993;1
18/02/03 14:46:38.704557; *t;1;62;524355994;1
18/02/03 14:46:40.493617; *t;0;69;524355997;1
18/02/03 14:46:42.365130; *t;0;72;524355998;1
18/02/03 14:46:44.108207; *t;0;74;524356000;1

18/02/03 14:46:45.841418; *t;0;76;524356001;1
18/02/03 14:46:47.648128; *t;0;84;524356002;2
18/02/03 14:46:49.469368; *t;0;112;524356004;3
18/02/03 14:46:51.241391; *t;0;114;524356006;2
18/02/03 14:46:53.013456; T;1;114;524356015;7906
18/02/03 14:46:55.534536; *t;0;0;524356016;12
*/

//#define DEBUG_MSG

#define pinSV 2
#define pinSV_irq 0 //IRQ that matches to pin 2

#define pinTV 3
#define pinTV_irq 1 //IRQ that matches to pin 3

#define MIN_PULSE_TIME_MS 600 //350  // delka cerneho ramecku je 700ms (600 + debounce time 80 je cca 680
#define DEBOUNCE_TIME_MS 80   //10 //doba ustaleni
unsigned long lpulse = 0;

#define REPORT_INTERVAL 15000 // report count pulses in 15sec interval
unsigned long last_reported_time=0;

typedef struct {
  volatile int level;
  volatile int pulseCount;
  volatile unsigned long time;
} EVENT;

/*
 * state variable for aggregation
 */
int SV_pulse_count = 0;
int TV_pulse_count = 0;

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

/*
 * reports pulse counts for emoncms hub
 */
void reportPulseCounts(){  
  /*
   * report data to hub
   */
  if ((millis()-last_reported_time>REPORT_INTERVAL) && ((SV_pulse_count!=0) || (TV_pulse_count!=0))) {
   Serial.print(SV_pulse_count);Serial.print(' ');
   Serial.print(TV_pulse_count);Serial.print(' ');
   Serial.print(millis());Serial.print(' ');
   Serial.print(millis()-last_reported_time);Serial.print(' ');

   Serial.println();
   //start counting from zero
   SV_pulse_count=TV_pulse_count=0;
   last_reported_time=millis();
  }
}

/*
 * false edge filter
 * returns true if puls was accepted , false when rejected
 */
int pulse(EVENT *event, char type ) {
  if (millis()-event->time<MIN_PULSE_TIME_MS) {
    //reject puls it is to short
#ifdef DEBUG_MSG
    Serial.print('!');
    printPulse(event, type);
#endif    
    return false;
  }
  
#ifdef DEBUG_MSG
  printPulse(event, type);
#endif    
  /*
   * aggregate pulses
   */
  if (type=='S') {
   SV_pulse_count++;
  } else {
   TV_pulse_count++;
  }
   
  return true;
}

/**
 * detects edges and sends pulses on each edge
 */
void vodaCheck(EVENT *lastEvent, EVENT *irqEvent, char type, int pin) {
  EVENT currentEvent;
  
  cli();//disable interrupts
  memcpy (&currentEvent, irqEvent, sizeof (EVENT));
  sei();//enable interrupts

  currentEvent.level=digitalRead(pin);

  /*
   * detekce hrany + debounncing
   *  1 jak dlouho to je od minuleho IRQ - debounding
   *  2 preklopila se uroven signalu od minule 
   *  pokud jsou podminky 1 a 2 splneny pak generuj puls (ktery oznacuje hranu signalu
   */  
  if ((millis() - currentEvent.time > DEBOUNCE_TIME_MS) && (currentEvent.level != lastEvent->level)) { 
      cli();//disable interrupts      
      resetEvent(irqEvent,pin);  //in case of no IRQ occurence since last report, this happens, when 
      sei();//enable interrupts
      
      
      if (pulse(lastEvent, type)) {
        memcpy (lastEvent, &currentEvent, sizeof (EVENT));
      }
  }  else {
    /*
     * this branch is only for debugginig to see what happends during debouncing period
     * print data only if there is change in number of irq events
     */
#ifdef DEBUG_MSG      
    if ((currentEvent.pulseCount != lastEvent ->pulseCount)) {
      Serial.print('*');
      printPulse(&currentEvent, type + 'a' - 'A');
      lastEvent ->pulseCount = currentEvent.pulseCount;
    }
#endif          
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
  reportPulseCounts();
  //delay(25);
}

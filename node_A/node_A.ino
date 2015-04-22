
//
//
// hypernet node A 1.2
//
//

#include <dht11.h>
#include "RF24Network.h"
#include "RF24.h"
#include <SPI.h>
#include "printf.h"
#include <JeeLib.h>

//////////////////////////////////////
// Create the structures for data
//////////////////////////////////////
struct data_to_send {
  unsigned node;
  unsigned temp;
  unsigned humi;
  unsigned light;
  unsigned door;
  unsigned long hkey;
  unsigned test; //why isn't it working ?
};

struct data_received {
  unsigned long hkey = 0;
};

//////////////////////////////////////
// Setup nRF24L01+ module
//////////////////////////////////////
RF24 radio(8,7);
RF24Network rf24Net(radio);
const uint64_t this_node =  01; // node A
const uint64_t other_node = 00; // base

//////////////////////////////////////
// Setup the rest of it
//////////////////////////////////////
dht11 DHT11;
#define DHT11PIN 4

const int optoPin   = A0;
const int doorPinA  = 2;
const int led_A_Pin = 6;
const int led_B_Pin = 5;
volatile bool led_B_state = LOW;
const int buzzPin   = 9;
volatile bool buzzPin_state = LOW;
unsigned long start_buzz_time = 0;
volatile bool soft_alarm_state = false;

unsigned long currentTime = 0;
unsigned long startTime   = 0;
bool runOnce = true;

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup the watchdog

//////////////////////////////////////
// setup
//////////////////////////////////////
void setup(void) {

  // Initialize interfaces
  printf_begin();
  Serial.begin(9600);
  Serial.print(F("Starting interfaces..."));
  SPI.begin();
  radio.begin();
  Serial.print(F("complete]\n"));
  rf24Net.begin(90, this_node);
  radio.setDataRate(RF24_250KBPS);
  radio.printDetails();

  // initialize pins
  pinMode(led_A_Pin, OUTPUT);
  pinMode(led_B_Pin, OUTPUT);
  pinMode(buzzPin, OUTPUT);
  pinMode(doorPinA, INPUT_PULLUP);

  // connect D2 pin as an External Interrupt - Reed switch
  attachInterrupt(0, doorStateChange, CHANGE);
}

//////////////////////////////////////
// loop
//////////////////////////////////////
uint32_t txTimer = 0;

void loop(void)
{

  // check If soft_alarm_state is true, then count 1s and set it to LOW
  if (soft_alarm_state){
    if (runOnce) {
      startTime = millis();
      runOnce = false;
      digitalWrite(led_B_Pin, HIGH);
      digitalWrite(buzzPin, HIGH);
    }
    currentTime = millis();

    if (currentTime - startTime <= 1000) {
      Serial.println(currentTime - startTime);
    }
    else {
        runOnce = true;
        digitalWrite(led_B_Pin, LOW);
        digitalWrite(buzzPin, LOW);
        soft_alarm_state = false;
    }

  }

  rf24Net.update();

  if (millis() - txTimer > 1000) {
    DHT11.read(DHT11PIN);
    data_to_send payload;
    payload.node  = 01;
    payload.temp  = DHT11.temperature;
    payload.humi  = DHT11.humidity;
    payload.light = map(analogRead(optoPin), 0, 1023, 100, 0);
    payload.door  = soft_alarm_state;
    payload.hkey  = random(10000000, 99999999);

    // starts counting
    unsigned long elapsed = 0;
    unsigned long start = millis();
    Serial.println(payload.hkey);

    // sends packet
    RF24NetworkHeader header(other_node);

    Serial.println("Sending data to base every 30 sec.");
    txTimer = millis();
    bool ok = rf24Net.write( header, &payload, sizeof(payload) );
    if (ok) {
      unsigned long finish = millis();
      elapsed = finish - start;
      Serial.println("ok.");
      Serial.print(elapsed);
      Serial.print("ms\n");
      rf24Net.update();
    }
    else {
      Serial.println("failed.");
    }
  }

  while ( rf24Net.available() )
  {
    Serial.println("Received packet");
    RF24NetworkHeader header2;
    data_received confirm;
    rf24Net.read(header2, &confirm, sizeof(confirm));
    Serial.println(confirm.hkey);
    Serial.println("------------------------------");
    if (!soft_alarm_state){
      Serial.println(F("Going to sleep..."));
      delay(100);
      radio.powerDown();
      Sleepy::loseSomeTime(30000);
      radio.powerUp();
      Serial.println(F("Woke up!"));
    }
  }


}

//////////////////////////////////////
// Interrupt for reed
//////////////////////////////////////
void doorStateChange() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 500)
  {
    soft_alarm_state = true;
  }
  last_interrupt_time = interrupt_time;
}

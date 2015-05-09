//
//
// hypernet base A 1.1
//
//

#include "U8glib.h"
#include "RF24Network.h"
#include "RF24.h"
#include <SPI.h>
#include "printf.h"

//////////////////////////////////////
// Create the structures for data
//////////////////////////////////////
struct data_received {
  unsigned node;
  unsigned temp;
  unsigned humi;
  unsigned light;
  unsigned door;
  unsigned pir;
  unsigned long hkey;
};

struct data_to_send {
  unsigned long hkey = 0;
}confirm;

unsigned long finish = 0;
unsigned long elapsed = 0;

//////////////////////////////////////
// Setup nRF24L01+ module & Oled
//////////////////////////////////////
RF24 radio(7,8);
RF24Network rf24Net(radio);
const uint64_t this_node  = 00; // base
const uint64_t other_node = 01; // node A
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

void clearDisplay(void){
  u8g.firstPage();
  do {
   // do nothing
  } while( u8g.nextPage() );
}

//////////////////////////////////////
// Setup the rest of it
//////////////////////////////////////
const int ledPin = 2;

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

  pinMode(ledPin, OUTPUT);
}

//////////////////////////////////////
// loop
//////////////////////////////////////
void loop() {

  rf24Net.update();
  data_received payload;

  // listen for incoming clients
  while ( rf24Net.available() )
    {
      RF24NetworkHeader header;
      confirm.hkey = 0;

      // read the next available header
      //rf24Net.peek(header);

      // print NRF24 header data
      String rf24header = "\n[header info => id: "
                        + String(header.id) + " type: "
                        + header.type + " from_node: "
                        + header.from_node +"]";
      Serial.println(rf24header);

      // read data
      rf24Net.read( header, &payload, sizeof(payload) );

      // Print data received
      String dataPrint = "[NODE: "  + String(payload.node) +" ]=>  "
                       + "[temp: "  + String(payload.temp) + "*C] "
                       + "[humi: "  + String((float)payload.humi,2) + "%] "
                       + "[light: " + String((float)payload.light,2) + "%] "
                       + "[door: "  + String(payload.door) + "] "
                       + "[hkey: "  + String(payload.hkey) + "] ";
      Serial.println(dataPrint);

      // send back data to confirm integrity
      confirm.hkey = payload.hkey;
      Serial.println("...");
      RF24NetworkHeader header2(header.from_node);

      // starts counting just before sending
      elapsed = 0;
      unsigned long start = millis();

      bool sent_back = rf24Net.write( header2, &confirm, sizeof(confirm));

      if (sent_back == true){
        finish = millis();
        elapsed = finish - start;
        Serial.print(elapsed);
        Serial.print("ms");
        Serial.print(F(" - Confirmed back \n"));
      }
      else{
        Serial.print(F("Not confirmed"));
      }
      rf24Net.update();
    }

    u8g.firstPage();
    do {
         u8g.setColorIndex(1);
         u8g.setFont(u8g_font_fixed_v0r);
         u8g.setPrintPos(1,10);
         u8g.print(F("HYPERNET"));
         u8g.setPrintPos(95,10);
         if (elapsed > 0 && elapsed <= 10){
           u8g.print("<<<<<");
         }
         else if (elapsed > 10 && elapsed <= 20){
           u8g.print(" <<<<");
         }
         else if (elapsed > 20 &&elapsed  <= 30){
           u8g.print("  <<<");
         }
         else if (elapsed > 30 && elapsed <= 40){
           u8g.print("   <<");
         }
         else if (elapsed == 0){
           u8g.print("xxxxx");
         }
         else {
           u8g.print("    <");
         }
         // --------------------------------
         String node_str;
         if (payload.door || payload.pir){
           digitalWrite(ledPin, HIGH);
           node_str = String("NODE #")+payload.node+String("     SOFT ALARM!");
         }
         else{
           digitalWrite(ledPin, LOW);
           node_str = String("NODE #")+payload.node;
         }

         u8g.setPrintPos(1,23);
         u8g.print(node_str);

         // --------------------------------
         String read_str1;
         read_str1 =
            String("T:")
            +payload.temp
            +String("\'C  ")
            +String("H:")
            +payload.humi
            +String("%  ")
            +String("L:")
            +payload.light
            +String("%  ");

         u8g.setPrintPos(1,36);
         u8g.print(read_str1);
         // --------------------------------
         String read_str2;
         read_str2 =
            String("Time elapsed:")
            +elapsed
            +String("ms");

         u8g.setPrintPos(1,48);
         u8g.print(read_str2);
         // --------------------------------
         u8g.setPrintPos(1,62);
         u8g.print(F("Hash key:"));
         u8g.setPrintPos(76,62);
         u8g.print(payload.hkey);
         u8g.setPrintPos(102,62);
         u8g.print("");

    } while( u8g.nextPage() );

    Serial.println("Listening for nodes..");

}

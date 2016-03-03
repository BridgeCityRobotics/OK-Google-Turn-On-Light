/*
 * PIR sensor tester
 */

// Includes
#include "RF24.h"
#include "RF24Network.h"
    // In RF24Network_config.h, Line 80, Must change 
        //  #include <RF24_config.h> 
        //  to 
        //  #include "RF24_config.h"
#include "RF24Mesh.h"
    // In RF24Mesh.h, Line 47, Must change 
        //  #include <RF24.h> 
        //  to 
        //  #include "RF24.h"
        //  and
        //  #include <RF24Network.h>
        //  to
        //  #include "RF24Network.h"
#include <SPI.h>
#include <EEPROM.h>

// Variables 
int ledPin = 2;                // choose the pin for the LED
int pirInputPin = 8;            // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int pirInputVal = 0;            // variable for reading the pin status
#define nodeID 1                // NRF24L01 Node Identification.  Must be a unique value from 1-255
uint32_t displayTimer = 0;      // Timer for sending out messages to the mesh network
struct payload_t {              //
  unsigned long ms;
  unsigned long counter;
};


// Configure the NRF24L01 CE and CS pins
RF24 radio(9, 10);               // Arduino SPI pins for CE and CS
RF24Network network(radio);
RF24Mesh mesh(radio, network);
 
void setup() {
  pinMode(ledPin, OUTPUT);      // declare LED as output
  pinMode(pirInputPin, INPUT);     // declare sensor as input
 
  Serial.begin(115200);         // Start the serial connection

  // Connect to the mesh
  mesh.setNodeID(nodeID);       // Set the nodeID manually
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();
}
 
void loop(){
  //
  // ==== READ THE PIR ====
  //
  pirInputVal = digitalRead(pirInputPin);  // read input value
  if (pirInputVal == HIGH) {            // check if the input is HIGH
    digitalWrite(ledPin, HIGH);  // turn LED ON
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } else {
    digitalWrite(ledPin, LOW); // turn LED OFF
    if (pirState == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }

  //
  // ==== UPDATE THE MESH NETWORK ====
  //
  mesh.update();

  // Send the current state to the master node every second
  if (millis() - displayTimer >= 1000) {
    displayTimer = millis();

    // Send an 'M' type message containing the current millis()
    // 'M' is a user-defined (1-127) message header_type to send. Used to distinguish between different types of data being transmitted
    if (!mesh.write(&pirState, 'M', sizeof(pirState))) {

      // If a write fails, check connectivity to the mesh network
      if ( ! mesh.checkConnection() ) {
        //refresh the network address
        Serial.println("Renewing Address");
        mesh.renewAddress();
      } else {
        Serial.println("Send fail, Test OK");
      }
    } else {
      Serial.print("Send OK: "); Serial.println(pirState);
    }
  }

  // Receive confirmation from Master
  while (network.available()) {
    RF24NetworkHeader header;
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    Serial.print("Received packet #");
    Serial.print(payload.counter);
    Serial.print(" at ");
    Serial.println(payload.ms);
  }  
}

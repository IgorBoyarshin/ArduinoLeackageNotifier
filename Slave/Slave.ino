#include <VirtualWire.h>
#include <LowPower.h>

const int LED_SENDING = 13;
const int LED_ALARM = 12;

const int SENSOR_ANALOG_PIN = A0;
const int WATER_CONTROL_PIN = 2;
const int TX_PIN = 3;
const int INTERVAL = 2000;
const int WATER_THRESHOLD = 250;
const uint8_t CODE_OK = B01101010;
const uint8_t CODE_ALARM = B10111100;

void setup() { 
  vw_set_tx_pin(TX_PIN);
  vw_setup(2000);

  pinMode(LED_SENDING, OUTPUT);
  pinMode(LED_ALARM, OUTPUT);
  pinMode(WATER_CONTROL_PIN, OUTPUT);
}


void loop() {
  digitalWrite(WATER_CONTROL_PIN, HIGH);
  delay(2);
  int level = analogRead(SENSOR_ANALOG_PIN);
  digitalWrite(WATER_CONTROL_PIN, LOW);  
   
  if (level > WATER_THRESHOLD) {
    digitalWrite(LED_SENDING, HIGH);
    digitalWrite(LED_ALARM, HIGH);

    vw_send((uint8_t*) &CODE_ALARM, 1);
    vw_wait_tx();
    
    digitalWrite(LED_SENDING, LOW);
     
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);   
  } else { 
    digitalWrite(LED_ALARM, LOW);
    digitalWrite(LED_SENDING, HIGH);
  
    vw_send((uint8_t*) &CODE_OK, 1);
    vw_wait_tx();
  
    digitalWrite(LED_SENDING, LOW);
    
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  }  
}

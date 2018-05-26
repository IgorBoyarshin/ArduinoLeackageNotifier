#include <VirtualWire.h>
#include <SoftwareSerial.h>
#include <LowPower.h>

//-----------------------------------------------------------------------------
// GSM SIM800 module
const uint8_t SIM800_RX_PIN = 8;
const uint8_t SIM800_TX_PIN = 9;
SoftwareSerial SIM800(SIM800_RX_PIN, SIM800_TX_PIN);

//const String telephoneNumber = "+380965128877";
const String TELEPHONE_NUMBER = "+380506527183";
const String LEACKAGE_MESSAGE_MASTER = "Leackage in master!!!";
const String LEACKAGE_MESSAGE_SLAVE = "Leackage in slave!!!";

const uint8_t SIM_CONTROL_PIN = 4;
//-----------------------------------------------------------------------------
const uint8_t LED_RECEIVING_PIN = 13;
const uint8_t LED_ALARM_PIN = 12;

const uint8_t RX_PIN = 3;
const uint8_t RX_CONTROL_PIN = 6;
const uint8_t CODE_OK = B01101010;
const uint8_t CODE_ALARM = B10111100;

const uint8_t WATER_SENSOR_PIN = A1;
const uint8_t WATER_CONTROL_PIN = 2;
const uint8_t WATER_THRESHOLD = 250;
//-----------------------------------------------------------------------------
bool masterLeackage = false;
bool slaveLeackage = false;
bool smsMasterSent = false;
bool smsSlaveSent = false;
//-----------------------------------------------------------------------------
void setup() {
    vw_set_rx_pin(RX_PIN);
    const uint16_t COMM_SPEED = 2000;
    vw_setup(COMM_SPEED);
    vw_rx_start();

    pinMode(LED_RECEIVING_PIN, OUTPUT);
    pinMode(LED_ALARM_PIN, OUTPUT);
    pinMode(WATER_CONTROL_PIN, OUTPUT);
    pinMode(RX_CONTROL_PIN, OUTPUT);
    pinMode(SIM_CONTROL_PIN, OUTPUT);
}
//-----------------------------------------------------------------------------
void loop() {
    // Check master water level

    digitalWrite(WATER_CONTROL_PIN, HIGH);
    delay(2); // just in case the device needs some time to turn on
    const uint16_t waterLevel = analogRead(WATER_SENSOR_PIN);
    digitalWrite(WATER_CONTROL_PIN, LOW);

    if (waterLevel > WATER_THRESHOLD) {        
        digitalWrite(LED_ALARM_PIN, HIGH);
        if (!smsMasterSent) {
            smsMasterSent = true;
            turnSimAndSendSms(LEACKAGE_MESSAGE_MASTER);            
        }
        masterLeackage = true;
    } else {
        masterLeackage = false;
    }

    // Check slave water level (receive message)

    uint8_t buff[VW_MAX_MESSAGE_LEN];
    uint8_t buffLength = VW_MAX_MESSAGE_LEN; // receive no more than this

    digitalWrite(RX_CONTROL_PIN, HIGH);
    delay(2); // just in case the device needs some time to turn on
    for (unsigned int i = 0; i < 5; i++) {
      delay(500);
      if (vw_get_message(buff, &buffLength)) {
          digitalWrite(LED_RECEIVING_PIN, HIGH);
          delay(2); // to make the LED more visible
          switch (buff[0]) {
              case CODE_OK:
                  slaveLeackage = false;
                  break;
              case CODE_ALARM:                  
                  digitalWrite(LED_ALARM_PIN, HIGH);
                  if (!smsSlaveSent) {
                      smsSlaveSent = true;
                      turnSimAndSendSms(LEACKAGE_MESSAGE_SLAVE);
                  }
                  slaveLeackage = true;
                  break;
              default:
                  // Received incorrect message
                  break;
          }
          digitalWrite(LED_RECEIVING_PIN, LOW);
      }
    }
    digitalWrite(RX_CONTROL_PIN, LOW);

    if (!(masterLeackage || slaveLeackage)) { // all good
        smsMasterSent = false; // reset flag for next leackage
        smsSlaveSent = false;
        digitalWrite(LED_ALARM_PIN, LOW);
        digitalWrite(SIM_CONTROL_PIN, LOW);
        LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);        
    } else { // all FUBAR
        //LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
        delay(1000);
        if (SIM800.available()) {
          SIM800.readString();
        }        
    }
}
//-----------------------------------------------------------------------------
void turnSimAndSendSms(String message) {  
    digitalWrite(SIM_CONTROL_PIN, HIGH);    
  if (!(masterLeackage || slaveLeackage)) { // if the module is not already running
    delay(1000); // just in case the device needs some time to turn on
    initSim();
  }
    sendSms(TELEPHONE_NUMBER, message);
    //waitResponse();

//    digitalWrite(SIM_CONTROL_PIN, LOW);
}
//-----------------------------------------------------------------------------
void initSim() {
    const uint16_t COMM_SPEED = 9600;
    SIM800.begin(COMM_SPEED);

    sendAtCommand("AT", true); // check-check
    sendAtCommand("AT+CMGF=1;&W", true); // enter text mode
}
//-----------------------------------------------------------------------------
void sendSms(String phone, String message) {
    // Prepare to input message text(after ">")
    sendAtCommand("AT+CMGS=\"" + phone + "\"", true);
    // Enter message text
    sendAtCommand(message + (String)((char)0x1A), true); // must end with 0x1A
}
//-----------------------------------------------------------------------------
String sendAtCommand(String cmd, bool waitForResponse) {
    SIM800.println(cmd);

    String response = "";
    if (waitForResponse) {
        response = waitResponse();
    }

    return response;
}
//-----------------------------------------------------------------------------
String waitResponse() {
    const unsigned long ONE_SECOND = 1000;
    const unsigned long TIMEOUT = millis() + 30 * ONE_SECOND;

    while (!SIM800.available() && (millis() < TIMEOUT));

    String response = "";
    if (SIM800.available()) {
        response = SIM800.readString();
    } // else === response timed out

    return response;
}


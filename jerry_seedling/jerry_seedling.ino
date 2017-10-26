#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "RTClib.h"

// PINS
#define PIN_LCD_RS 12
#define PIN_LCD_EN 11
#define PIN_LCD_D4 5
#define PIN_LCD_D5 4
#define PIN_LCD_D6 3
#define PIN_LCD_D7 2
#define PIN_ONEWIRE 10
#define PIN_RLY_1 6
#define PIN_RLY_2 7
#define PIN_RLY_3 8
#define PIN_RLY_4 9
#define PIN_BTN_SENSE A0


// TEMP SENSORS
#define TEMPERATURE_PRECISION 11
#define TEMPERATURE_UPDATE_DELAY 1000
OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensors(&oneWire);
DeviceAddress TEMP_INT_ADDR = {0x28, 0x7E, 0xE4, 0x4D, 0x07, 0x00, 0x00, 0x57};
DeviceAddress TEMP_OUT_ADDR = {0x28, 0xFB, 0x17, 0x2B, 0x06, 0x00, 0x00, 0x8C};
float tempIntC = 0;
float tempOutC = 0;
unsigned long tempRequestTime = 0;
unsigned long tempRequestDelay = 1000;

// RTC
#define RTC_UPDATE_DELAY 2000
RTC_DS1307 RTC;
DateTime now;
unsigned long rtcRequestTime = 0;

// FRONT PANEL BUTTONS

#define BTN_UPDATE_DELAY 5
#define BTN_DEBOUNCE 8
#define BTN_COUNT 6

#define BTN_STATE_ERR -1
#define BTN_STATE_NONE 0
#define BTN_STATE_UP 1
#define BTN_STATE_DOWN 2
#define BTN_STATE_LEFT 3
#define BTN_STATE_RIGHT 4
#define BTN_STATE_SELECT 5

int buttonValue = 0;
int buttonState = BTN_STATE_NONE;
unsigned int buttonUpdateTime = 0;
int buttonNewState = BTN_STATE_NONE;
int buttonDebounceCount = 0;

int BTN_RANGES[BTN_COUNT][2] = {
    {0, 30},
    {500, 520},
    {100, 120},
    {310, 330},
    {200, 220},
    {390, 420}
};

// FRONT PANEL LCD

#define LCD_UPDATE_DELAY 200
LiquidCrystal lcd(
    PIN_LCD_RS, 
    PIN_LCD_EN, 
    PIN_LCD_D4, 
    PIN_LCD_D5, 
    PIN_LCD_D6, 
    PIN_LCD_D7
);
unsigned long lcdUpdateTime = 0;
byte ICON_DATA_LIGHT[8] = {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b10101,
    0b10101,
    0b01010,
    0b01110,
};

byte ICON_DATA_TEMP[8] = {
    0b00100,
    0b01010,
    0b01010,
    0b01110,
    0b01110,
    0b10101,
    0b10001,
    0b01110
};

byte ICON_DATA_HOME[8] = {
    0b00000,
    0b00100,
    0b01010,
    0b11011,
    0b01010,
    0b01010,
    0b01110,
    0b00000
};

byte ICON_LIGHT = byte(0);
byte ICON_TEMP = byte(1);
byte ICON_HOME = byte(2);

#define VIEW_COUNT 3
#define VIEW_HOME 0
#define VIEW_TEMP 1
#define VIEW_LIGHT 2

int viewState = VIEW_HOME;


// LIGHT RELAY
#define PIN_RLY_LIGHT PIN_RLY_1

#define LIGHT_STATE_COUNT 5
#define LIGHT_STATE_OFF 0
#define LIGHT_STATE_ON 1
#define LIGHT_STATE_AUTO_24 2
#define LIGHT_STATE_AUTO_18 3
#define LIGHT_STATE_AUTO_12 4

int lightState = LIGHT_STATE_AUTO_24;

int lightOutput = LOW;



void setup(){

    // INIT SERIAL
    Serial.begin(9600);
    Serial.println("Jerry Seedling Running...");

    // INIT I/O PINS
    pinMode(PIN_RLY_1, OUTPUT);
    pinMode(PIN_RLY_2, OUTPUT);
    pinMode(PIN_RLY_3, OUTPUT);
    pinMode(PIN_RLY_4, OUTPUT);
    digitalWrite(PIN_RLY_1, LOW);
    digitalWrite(PIN_RLY_2, LOW);
    digitalWrite(PIN_RLY_3, LOW);
    digitalWrite(PIN_RLY_4, LOW);

    pinMode(PIN_BTN_SENSE, INPUT);

    // INIT RTC
    Wire.begin();
    RTC.begin();
    
    if (! RTC.isrunning()) {
        Serial.println("RTC is NOT running!");
        // following line sets the RTC to the date & time this sketch was compiled
        RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    // INIT TEMP SENSORS
    sensors.begin();
    // sensors.getAddress(TEMP_INT_ADDR, 0);
    sensors.setResolution(TEMP_INT_ADDR, TEMPERATURE_PRECISION);
    // sensors.getAddress(TEMP_OUT_ADDR, 1);
    sensors.setResolution(TEMP_OUT_ADDR, TEMPERATURE_PRECISION);
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
    tempRequestDelay = 750 / (1 << (12 - TEMPERATURE_PRECISION));
    tempRequestTime = millis();

    // INIT LCD
    lcd.begin(16, 2);
    lcd.createChar(ICON_LIGHT, ICON_DATA_LIGHT);
    lcd.createChar(ICON_TEMP, ICON_DATA_TEMP);
    lcd.createChar(ICON_HOME, ICON_DATA_HOME);
    
    lcd.setCursor(0, 0);
    lcd.print("     Jerry      ");
    lcd.setCursor(0, 1);
    lcd.print("    Seedling    ");
    
    
    delay(1000);
    lcd.clear();
    // findOneWireDevices();
}


void loop(){
    unsigned long startTime = millis();
    updateButtonInput(startTime);
    updateLight();
    updateTemp(startTime);
    updateRTC(startTime);
    updateLCD(startTime);
    updateRelays();
    // printPlot();
    // unsigned long endTime = millis();
    // Serial.print("Loop took ");
    // Serial.println(millis() - startTime);


    delay(5);
}

void printPlot(){
  Serial.print(tempIntC);
  Serial.print("\t");
  Serial.print(tempOutC);
  Serial.print("\t");
  Serial.println(buttonValue);
}

void updateButtonInput(unsigned long startTime){

    unsigned int timeSinceLast = startTime - buttonUpdateTime;

    if(timeSinceLast < BTN_UPDATE_DELAY){
        return;
    }

    buttonValue = analogRead(PIN_BTN_SENSE);
    int state = BTN_STATE_ERR;
    for(int b = 0; b < BTN_COUNT; b++){
      if(buttonValue >= BTN_RANGES[b][0] && buttonValue <= BTN_RANGES[b][1]){
        state = b;
        break;
      }
    }

    if(state == buttonNewState){
        buttonDebounceCount++;
        if(buttonDebounceCount > BTN_DEBOUNCE && state != buttonState){
            updateButtonState(state);
        }
    } else {
        buttonDebounceCount = 0;
        buttonNewState = state;
    }

    buttonUpdateTime = millis();

    
    // Serial.print('\r');
    // Serial.print("Button ");
    // Serial.println(buttonValue);
}

void updateButtonState(int newState){
    buttonState = newState;

    switch(newState){
        case BTN_STATE_LEFT:
            viewState--;
            if(viewState < 0) viewState = VIEW_COUNT - 1;
            break;
        case BTN_STATE_RIGHT:
            viewState++;
            if(viewState >= VIEW_COUNT) viewState = 0;
            break;
        case BTN_STATE_UP:
            switch(viewState){
                case VIEW_LIGHT:
                    lightState--;
                    if(lightState < 0) lightState = LIGHT_STATE_COUNT - 1;
                    break;
                case VIEW_TEMP:
                    break;
            }
            break;
        case BTN_STATE_DOWN:
            switch(viewState){
                case VIEW_LIGHT:
                    lightState++;
                    if(lightState >= LIGHT_STATE_COUNT) lightState = 0;
                    break;
                case VIEW_TEMP:
                    break;
            }
            break;
        case BTN_STATE_SELECT:
            viewState = VIEW_HOME;
            break;

    }


}

void updateRelays(){
    digitalWrite(PIN_RLY_LIGHT, lightOutput); 

}

void updateLight(){

    switch(lightState){
        case LIGHT_STATE_AUTO_24:
        case LIGHT_STATE_AUTO_18:
        case LIGHT_STATE_AUTO_12:
            // TODO: determine light state
            lightOutput = HIGH;
            break;
        case LIGHT_STATE_ON:
            lightOutput = HIGH;
            break;  
        case LIGHT_STATE_OFF:
            lightOutput = LOW;
            break;
    }

}



/**
 * Update LCD Display
 * 
 * Takes 10-11ms
 * 
 */
void updateLCD(unsigned long startTime){

    unsigned int timeSinceLast = startTime - lcdUpdateTime;

    if(timeSinceLast < LCD_UPDATE_DELAY){
        return;
    }


    // unsigned long startTime = millis();

    char buffer[16];
    char floatBuffer[7];
    int hour = now.hour();
    int minute = now.minute();

    // line 1
    lcd.setCursor(0,0);

    switch(viewState){
        case VIEW_HOME:
            lcd.write(ICON_HOME);
            sprintf(buffer, "%02d:%02d   ", hour, minute);
            lcd.print(buffer);
            dtostrf(tempIntC, 6, 2, floatBuffer);
            lcd.print(floatBuffer);
            lcd.write(ICON_TEMP);
            lcd.setCursor(0,1);
            lcd.write(ICON_LIGHT);
            if(lightState){
                lcd.print("ON ");
            }else{
                lcd.print("OFF");
            }
            lcd.print("            ");
            break;
        case VIEW_LIGHT:
            lcd.write(ICON_LIGHT);

            switch(lightState){
                case LIGHT_STATE_OFF:
                    lcd.print("            OFF");
                    break;
                case LIGHT_STATE_ON:
                    lcd.print("             ON");
                    break;
                case LIGHT_STATE_AUTO_12:
                    lcd.print("           AUTO");
                    break;
                case LIGHT_STATE_AUTO_18:
                    lcd.print("           AUTO");
                    break;
                case LIGHT_STATE_AUTO_24:
                    lcd.print("           AUTO");
                    break;
                default:
                    lcd.print("            ERR");
                    break;
            }
            lcd.setCursor(0,1);
            switch(lightState){
                case LIGHT_STATE_AUTO_12:
                    lcd.print("          12/24h");
                    break;
                case LIGHT_STATE_AUTO_18:
                    lcd.print("          18/24h");
                    break;
                case LIGHT_STATE_AUTO_24:
                    lcd.print("          24/24h");
                    break;
                default:
                    lcd.print("                ");
                    break;
            }
            break;
        case VIEW_TEMP:
            lcd.write(ICON_TEMP);
            lcd.print("     IN ");
            dtostrf(tempIntC, 6, 2, floatBuffer);
            lcd.print(floatBuffer);
            lcd.print('C');
            lcd.setCursor(0,1);
            lcd.print("     OUT ");
            dtostrf(tempOutC, 6, 2, floatBuffer);
            lcd.print(floatBuffer);
            lcd.print('C');
            break;
        default:
            lcd.print("      ERR       ");
            lcd.setCursor(0,1);
            lcd.print("      404       ");
        break;
    }

    lcdUpdateTime = millis();
    // Serial.println("UPDATE LCD");
}


/**
 * Update temps from sensors if
 * conversion is complete.
 * 
 * Takes 0-1ms when skipping read
 * Takes 25-27ms when reading
 * 
 */
void updateTemp(unsigned long startTime){
    // unsigned long startTime = millis();
    unsigned int timeSinceLast = startTime - tempRequestTime;
    if (timeSinceLast >= TEMPERATURE_UPDATE_DELAY && timeSinceLast >= tempRequestDelay) // waited long enough??
    {
        tempIntC = sensors.getTempC(TEMP_INT_ADDR);
        tempOutC = sensors.getTempC(TEMP_OUT_ADDR);
        sensors.requestTemperatures();
        tempRequestTime = millis();
        // Serial.println("UPDATE TEMP");
    }
    // unsigned long endTime = millis();
    // Serial.print("TEMP UPT took ");
    // Serial.println(endTime - startTime);
}

void updateRTC(unsigned long startTime){
    unsigned int timeSinceLast = startTime - rtcRequestTime;
    if(timeSinceLast >= RTC_UPDATE_DELAY){
        now = RTC.now();
        rtcRequestTime = millis();
        // Serial.println("UPDATE RTC");
    }
}


// void findOneWireDevices(){

//     // locate devices on the bus
//     Serial.print("Locating devices...");
//     Serial.print("Found ");
//     Serial.print(sensors.getDeviceCount(), DEC);
//     Serial.println(" devices.");

//     // report parasite power requirements
//     Serial.print("Parasite power is: "); 
//     if (sensors.isParasitePowerMode()) Serial.println("ON");
//     else Serial.println("OFF");

//     if (!sensors.getAddress(TEMP_INT_ADDR, 0)) {
//         Serial.println("Unable to find address for Device 0"); 
//     }

//     if (!sensors.getAddress(TEMP_OUT_ADDR, 1)) { 
//         Serial.println("Unable to find address for Device 1"); 
//     }

//     Serial.print("Device 0 Address: ");
//     printAddress(TEMP_INT_ADDR);
//     Serial.println();

//     Serial.print("Device 1 Address: ");
//     printAddress(TEMP_OUT_ADDR);
//     Serial.println();
  
//     // set the resolution to 9 bit per device
//     sensors.setResolution(TEMP_INT_ADDR, TEMPERATURE_PRECISION);
//     sensors.setResolution(TEMP_OUT_ADDR, TEMPERATURE_PRECISION);
  
//     Serial.print("Device 0 Resolution: ");
//     Serial.print(sensors.getResolution(TEMP_INT_ADDR), DEC); 
//     Serial.println();
  
//     Serial.print("Device 1 Resolution: ");
//     Serial.print(sensors.getResolution(TEMP_OUT_ADDR), DEC); 
//     Serial.println();

//     // call sensors.requestTemperatures() to issue a global temperature 
//     // request to all devices on the bus
//     Serial.print("Requesting temperatures...");
//     sensors.requestTemperatures();
//     Serial.println("DONE");

//     // print the device information
//     printData(TEMP_INT_ADDR);
//     printData(TEMP_OUT_ADDR);

// }

// function to print a device address
// void printAddress(DeviceAddress deviceAddress)
// {
//   for (uint8_t i = 0; i < 8; i++)
//   {
//     // zero pad the address if necessary
//     if (deviceAddress[i] < 16) Serial.print("0");
//     Serial.print(deviceAddress[i], HEX);
//   }
// }

// function to print a device's resolution
// void printResolution(DeviceAddress deviceAddress)
// {
//   Serial.print("Resolution: ");
//   Serial.print(sensors.getResolution(deviceAddress));
//   Serial.println();    
// }

// void printData(DeviceAddress deviceAddress)
// {
//   Serial.print("Device Address: ");
//   printAddress(deviceAddress);
//   Serial.print(" ");
//   printTemperature(deviceAddress);
//   Serial.println();
// }

// function to print the temperature for a device
// void printTemperature(DeviceAddress deviceAddress)
// {
//   float tempC = sensors.getTempC(deviceAddress);
//   Serial.print("Temp C: ");
//   Serial.print(tempC);
//   Serial.print(" Temp F: ");
//   Serial.print(DallasTemperature::toFahrenheit(tempC));
// }

// void printDateTime(){
//     Serial.print(now.year(), DEC);
//     Serial.print('/');
//     Serial.print(now.month(), DEC);
//     Serial.print('/');
//     Serial.print(now.day(), DEC);
//     Serial.print(' ');
//     Serial.print(now.hour(), DEC);
//     Serial.print(':');
//     Serial.print(now.minute(), DEC);
//     Serial.print(':');
//     Serial.print(now.second(), DEC);
//     Serial.println(); 
// }

// void findOneWireDevices(){

//     byte i;
//     byte addr[8];

//     Serial.println("Getting the address...\n\r");
//     /* initiate a search for the OneWire object we created and read its value into
//     addr array we declared above */

//     while(oneWire.search(addr)){
//         Serial.print("The address is:\t");
//         //read each byte in the address array
//         for( i = 0; i < 8; i++) {
//           Serial.print("0x");
//           if (addr[i] < 16) {
//             Serial.print('0');
//           }
//           // print each byte in the address array in hex format
//           Serial.print(addr[i], HEX);
//           if (i < 7) {
//             Serial.print(", ");
//           }
//         }
//         // a check to make sure that what we read is correct.
//         if ( OneWire::crc8( addr, 7) != addr[7]) {
//             Serial.print("CRC is not valid!\n");
//             return;
//         }
//     }

//     oneWire.reset_search();
    
// }

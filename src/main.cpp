#include <Arduino.h>

#include "DHT.h"

#include <ThreeWire.h>
#include <RtcDS1302.h>

#include <ShiftRegister74HC595.h>
#ifndef ARDUINO_AVR_NANO_EVERY 
#include <LowPower.h>
#endif
#include <JC_Button.h>
/*===============================================================================================*/
// #define DEBUG //TODO comment this line in final build

#ifdef DEBUG
#define DEBUGPRINT(x) Serial.print(x)
#define DEBUGPRINTLN(x) Serial.println(x)
#define DEBUGSERIALBEGIN(x) Serial.begin(x)
#else
#define DEBUGPRINT(x)
#define DEBUGPRINTLN(x)
#define DEBUGSERIALBEGIN(x)
#endif
/*_______________________________________________________________________________________________*/
#define NO_OF_SHIFT_REGISTERS 6
#define SHIFT_REGISTER_DATA 2
#define SHIFT_REGISTER_CLK 3
#define SHIFT_REGISTER_LATCH 4
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
#define BATTERY_VOLTAGE_PIN A7
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
#define DHTPIN 10
#define DHTTYPE DHT11
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
#define RTC_CLOCK 6
#define RTC_DATA 5
#define RTC_RESET 7
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
#define COLON_RIGHT 8
#define COLON_LEFT 9
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
#define RED_TIME_BTN 14    // A0
#define YELLOW_DATE_BTN 15 // A2
#define GREEN_TEMP_BTN 16  // A1
#define BLUE_HMD_BTN 17    // A3
#define BLACK_VOLT_BTN 19  // A5
/*===============================================================================================*/

ThreeWire myWire(RTC_DATA, RTC_CLOCK, RTC_RESET); // DAT, CLK, RST
RtcDS1302<ThreeWire> Rtc(myWire);
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
// Create the Shiftregister
ShiftRegister74HC595<NO_OF_SHIFT_REGISTERS> sr(SHIFT_REGISTER_DATA, SHIFT_REGISTER_CLK,
                                               SHIFT_REGISTER_LATCH);
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
// Create the Temperature/Humidity Sensor
DHT dht(DHTPIN, DHTTYPE);
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
// Create the 5 Buttons
Button redTimeBtn(RED_TIME_BTN),
    yellowDateBtn(YELLOW_DATE_BTN),
    greenTempBtn(GREEN_TEMP_BTN),
    blueHmdBtn(BLUE_HMD_BTN),
    blackVoltBtn(BLACK_VOLT_BTN);
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
// Create a global array and pointing variable for the values to be shifted into the registers
volatile uint8_t pinValues[NO_OF_SHIFT_REGISTERS];
volatile uint8_t pinArrPtr = NO_OF_SHIFT_REGISTERS - 1;
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
// create another global array to be able to set decimal points
volatile uint8_t decimalPointArr[] = {6, 6};
/*===============================================================================================*/
enum statemachine_t
{
    TIME,
    DATE,
    TEMPERATURE,
    HUMIDITY,
    VOLT
};

statemachine_t mode = TIME;
/*===============================================================================================*/
// Parses numbers and character without implicit casting by using uint8_t
void uint8_tToBitMask(uint8_t value)
{
    switch (value)
    {
    case 0:
        pinValues[pinArrPtr] = B00001010; // 0                 0x0A
        break;
    case 1:
        pinValues[pinArrPtr] = B11101011; // 1                 0xEB
        break;
    case 2:
        pinValues[pinArrPtr] = B01000110; // 2                 0x46
        break;
    case 3:
        pinValues[pinArrPtr] = B11000010; // 3                 0xC2
        break;
    case 4:
        pinValues[pinArrPtr] = B10100011; // 4                 0xA3
        break;
    case 5:
        pinValues[pinArrPtr] = B10010010; // 5                 0x92
        break;
    case 6:
        pinValues[pinArrPtr] = B00010010; // 6                 0x12
        break;
    case 7:
        pinValues[pinArrPtr] = B11001011; // 7                 0xCB
        break;
    case 8:
        pinValues[pinArrPtr] = B00000010; // 8                 0x02
        break;
    case 9:
        pinValues[pinArrPtr] = B10000010; // 9                 0x82
        break;
    case 42:                              // Asterix as a replacement for the Degree symbol ASCII
        pinValues[pinArrPtr] = B10000111; // Degree            0x87
        break;
    case 45:                              // Minus Symbol ASCII
        pinValues[pinArrPtr] = B11110111; // Minus Symbol      0xF7
        break;
    case 47:                              // Slash Symbol ASCII
        pinValues[pinArrPtr] = B01100111; // Slash Symbol      0x67
        break;
    case 67:                              // Capital C
        pinValues[pinArrPtr] = B00011110; // C                 0x30
        break;
    case 69:                              // Capital E ASCII
        pinValues[pinArrPtr] = B00010110; // E                 0x16
        break;
    case 70:                              // Capital F ASCII
        pinValues[pinArrPtr] = B00010111; // F                 0x17
        break;
    case 111:                             // lower case O ASCII
        pinValues[pinArrPtr] = B01110010; // o                 0x72
        break;
    case 114:                             // lower case R ASCII
        pinValues[pinArrPtr] = B01110111; // r                 0x77
        break;
    case 116:                             // Lower case T ASCII
        pinValues[pinArrPtr] = B00110110; // t                 0x36
        break;
    case 118:                             // Upper Case V ASCII
        pinValues[pinArrPtr] = B01111010; // vV                0x7A
        break;
    default:                              // Turn Segment of
        pinValues[pinArrPtr] = B11111111; // No symbol all HIGH 0xFF
    }
}
/*_______________________________________________________________________________________________*/

void printError()
{
    uint8_t arr[6] = {'E', 'r', 'r', 'o', 'r', 128};

    pinArrPtr = 5; // remove all current entries from the list
    for (int i = 0; i < 6; ++i)
    {
        uint8_tToBitMask(arr[i]);
        pinArrPtr--;
    }
}
/*_______________________________________________________________________________________________*/

void getTimeBitmaskList(const RtcDateTime &dt)
{
    uint8_t arr[6];

    arr[0] = uint8_t(dt.Hour() / 10);   // Upper digit of the day
    arr[1] = uint8_t(dt.Hour() % 10);   // Lower digit of the day
    arr[2] = uint8_t(dt.Minute() / 10); // Upper digit of the day
    arr[3] = uint8_t(dt.Minute() % 10); // Lower digit of the day
    arr[4] = uint8_t(dt.Second() / 10); // Upper digit of the day
    arr[5] = uint8_t(dt.Second() % 10); // Lower digit of the day

    pinArrPtr = 5; // remove all current entries from the list

    for (int i = 0; i < 6; ++i)
    {
        uint8_tToBitMask(arr[i]);
        pinArrPtr--;
    }
}
/*_______________________________________________________________________________________________*/

void getDateBitmaskList(const RtcDateTime &dt)
{
    uint8_t arr[6];

    arr[0] = uint8_t(dt.Day() / 10);           // Upper digit of the day
    arr[1] = uint8_t(dt.Day() % 10);           // Lower digit of the day
    arr[2] = uint8_t(dt.Month() / 10);         // Upper digit of the month
    arr[3] = uint8_t(dt.Month() % 10);         // Lower digit of the month
    arr[4] = uint8_t((dt.Year() - 2000) / 10); // Upper digit of short year (year-2000)/10
    arr[5] = uint8_t((dt.Year() - 2000) % 10); // Lower digit of short year

    decimalPointArr[0] = 4;
    decimalPointArr[1] = 2;
    pinArrPtr = 5; // remove all current entries from the list
    for (int i = 0; i < 6; ++i)
    {
        uint8_tToBitMask(arr[i]);
        pinArrPtr--;
    }
}
/*_______________________________________________________________________________________________*/

void getBatVoltageBitmaskList()
{
    /* Calculate the battery voltage as an 4 Digit Integer with "hidden" decimal sign.
     * The remaining digets are cut of.
     * 12 = 1.5V*8 Batteries because the Voltage Divider divides the battery valtage by half.
     * 1023 = Analog Resoltion (2^10)-1 (10-Bits)
     */
    float bat_volt = (float)analogRead(BATTERY_VOLTAGE_PIN) * 12.0 / 1023.0;
    // Serial.println(bat_volt);

    uint8_t arr[6];

    // returns the integer in arr[1:4] leaving
    for (uint8_t i = 1; i < 4; ++i)
    {
        arr[i] = (uint8_t)bat_volt;
        // Serial.println(arr[i]);
        bat_volt = (bat_volt - arr[i]) * 10; // Substract the rounded value and multipy by 10
    }

    arr[0] = 128; // this will be caugt by the default case and will disable the segment
    arr[4] = 128;
    arr[5] = 'v';

    decimalPointArr[0] = 4;

    pinArrPtr = 5; // remove all current entries from the list
    for (int i = 0; i < 6; ++i)
    {
        uint8_tToBitMask(arr[i]);
        pinArrPtr--;
    }
}
/*_______________________________________________________________________________________________*/

void getDHTTemperatureBitmaskList()
{
    uint8_t arr[6];
    float temperature_float = dht.readTemperature();
    if (isnan(temperature_float))
    {
        printError();
    }
    else
    {
        int8_t temperature_int = (int8_t)temperature_float;
        if (temperature_int < 0) // When have a negative temperature
        {
            arr[0] = 128;                                // no symbole to be displayed
            arr[1] = (temperature_int < -9 ? '-' : 128); // When we have -10 degress or less than '-'
            // When -1 to -9 degress than '-' else the first digit of the temperature as a positive number
            arr[2] = (temperature_int >= -9 ? '-' : (uint8_t)(temperature_int / 10) * -1);
            arr[3] = (temperature_int >= -9 ? (uint8_t)(temperature_int / 10) * -1 : (temperature_int % -10) * -1);
            arr[4] = '*';
            arr[5] = 'C';
        }
        else
        {
            arr[0] = 128;
            arr[1] = 128;
            arr[2] = (temperature_int > 9 ? (uint8_t)temperature_int / 10 : 128);
            arr[3] = (temperature_int > 9 ? temperature_int % 10 : (uint8_t)temperature_int / 10);
            arr[4] = '*';
            arr[5] = 'C';
        }
        pinArrPtr = 5; // remove all current entries from the list
        for (int i = 0; i < 6; ++i)
        {
            uint8_tToBitMask(arr[i]);
            pinArrPtr--;
        }
    }
}
/*_______________________________________________________________________________________________*/

void getDHTHumidityBitmaskList()
{
    uint8_t arr[6];
    float humidity_float = dht.readHumidity();
    if (isnan(humidity_float))
    {
        printError();
    }
    else
    {
        uint8_t humidity_int = (uint8_t)humidity_float;
        arr[0] = (humidity_int > 9 ? (uint8_t)humidity_int / 10 : 128);
        arr[1] = (humidity_int > 9 ? humidity_int % 10 : (uint8_t)humidity_int / 10);
        arr[2] = 128;
        arr[3] = '*'; // Percent sign part 1
        arr[4] = '/'; // percent sign part 2
        arr[5] = 'o'; // percent sign part 3
    }
    pinArrPtr = 5; // remove all current entries from the list
    for (int i = 0; i < 6; ++i)
    {
        uint8_tToBitMask(arr[i]);
        pinArrPtr--;
    }
}
/*_______________________________________________________________________________________________*/

void updateButtonsAndSetState()
{
    redTimeBtn.read();
    yellowDateBtn.read();
    greenTempBtn.read();
    blueHmdBtn.read();
    blackVoltBtn.read();

    // changing the State depnding if a button was pressed or not.
    if (redTimeBtn.wasPressed())
    {
        mode = TIME;
    }
    else if (yellowDateBtn.wasPressed())
    {
        mode = DATE;
    }
    else if (greenTempBtn.wasPressed())
    {
        mode = TEMPERATURE;
    }
    else if (blueHmdBtn.wasPressed())
    {
        mode = HUMIDITY;
    }
    else if (blackVoltBtn.wasPressed())
    {
        mode = VOLT;
    }
}
/*===============================================================================================*/

void setup()
{
    // Disabling all Sengments
    sr.setAllHigh();
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    // Start the DHT Temperature, Humidity Sensor
    dht.begin();
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    // Start the external RTC
    Rtc.Begin();
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    // Start the 4 Buttons
    redTimeBtn.begin();
    yellowDateBtn.begin();
    greenTempBtn.begin();
    blueHmdBtn.begin();
    blackVoltBtn.begin();
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    pinMode(COLON_LEFT, OUTPUT);
    pinMode(COLON_RIGHT, OUTPUT);
    /*___________________________________________________________________________________________*/

    DEBUGSERIALBEGIN(9600);
    DEBUGPRINTLN("Clock Started");
    DEBUGPRINT(__DATE__);
    DEBUGPRINTLN(__TIME__);

#ifdef DEBUG
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
#endif
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    if (!Rtc.IsDateTimeValid())
    {
        DEBUGPRINTLN("Time Invalid");
        /* Common Causes:
         *    1) first time you ran and the device wasn't running yet
         *    2) the battery on the device is low or even missing
         **/
        printError();
#ifdef DEBUG
        Rtc.SetDateTime(compiled);
#endif
    }
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    if (Rtc.GetIsWriteProtected())
        Rtc.SetIsWriteProtected(false);

    if (!Rtc.GetIsRunning())
        Rtc.SetIsRunning(true);
/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
#ifdef DEBUG

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled)
        DEBUGPRINTLN("Time Updated");
    Rtc.SetDateTime(compiled);
#endif
}
/*===============================================================================================*/
void loop()
{
    RtcDateTime now = Rtc.GetDateTime();
    /* Serial get compleyly mess up with lowpower but well whatever. Also we frequently jump a a
     * number but it only seconds
     * and likly caused by the overhead of serial.
     */
    // Sleep 4 Times for a very short time and upate the button in between
    for (uint8_t i = 0; i < 4; i++)
    {
#ifdef DEBUG
        // DEBUGPRINTLN("Dealying for 250MS");
        delay(250); // Just delay fro 250MS
#elif ARDUINO_AVR_NANO_EVERY
        delay(250);
#else
        // Else actually sleep
        LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
#endif
        // Update the buttons and the State machine
        updateButtonsAndSetState();

        if ((mode == TIME) && ((i == 0) || (i == 1)))
        {
            // Enable the Colons every 500 ms
            digitalWrite(COLON_LEFT, LOW);
            digitalWrite(COLON_RIGHT, LOW);
        }
        else
        {
            // Disable the Colons every 500 ms
            digitalWrite(COLON_LEFT, HIGH);
            digitalWrite(COLON_RIGHT, HIGH);
        }
    }
    // Serial.print("Mode: ");
    // Serial.println(mode);
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    switch (mode)
    {
    case TIME:
        if (!now.IsValid())
        {
            printError();
            DEBUG_PRINTLN("Error!, Time is invalid");
        }
        else
        {
            DEBUG_PRINTLN("Getting TimeBitMask");
            getTimeBitmaskList(now);
        }
        break;
    case DATE:
        if (!now.IsValid())
        {
            printError();
            DEBUG_PRINTLN("Error!, Date is invalid");
        }
        else
        {
            DEBUG_PRINTLN("Getting DateBitMask");
            getDateBitmaskList(now);
        }
        break;
    case TEMPERATURE:
        DEBUG_PRINTLN("Getting TemperatureBitMask");
        getDHTTemperatureBitmaskList();
        break;
    case HUMIDITY:
        DEBUG_PRINTLN("Getting HumidityBitMask");
        getDHTHumidityBitmaskList();
        break;
    case VOLT:
        DEBUG_PRINTLN("Getting VoltageBitMask");
        getBatVoltageBitmaskList();
        break;
    default:
        DEBUGPRINTLN("Setting mode to TIME");
        mode = TIME; // When something bad happend we just reset the mode to TIME.
        break;
    }
    // getDateBitmaskList(now);
    /*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -  */
    // mutate Pin Values, if we have set a specific location for a decimal point;
    if ((decimalPointArr[0] < 6))
    {
        pinValues[decimalPointArr[0]] -= 2; // substracts 2 to set bit 2 to 0 instead of 1
        decimalPointArr[0] = 6;             // set the value back to 6
    }

    if ((decimalPointArr[1] < 6))
    {
        pinValues[decimalPointArr[1]] -= 2; // substracts 2 to set bit 2 to 0 instead of 1
        decimalPointArr[1] = 6;             // set the value back to 6
    }
    sr.setAll(pinValues);
}
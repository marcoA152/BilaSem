/////////biladatalogger/////////

////sensors libs e co
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#define ONE_WIRE_BUS 6 //DS18S20 Signal pin on digital 6
OneWire oneWire(ONE_WIRE_BUS);  // on digital pin 6
DallasTemperature sensors(&oneWire);

#include "GravityTDS.h"
GravityTDS gravityTds;

////sim800
#include <SoftwareSerial.h>
SoftwareSerial gprsSerial(10, 3);//SIM800L Tx & Rx is connected to Arduino #10 #3
const String THING_SPEAK_API_URL  = "GET https://api.thingspeak.com/update?api_key=ZZZZZZZZZZZZZZZZZZZZZZ";
String request_url = "";

/////clock
#include "RTClib.h"     // DS3231 library
#include <avr/sleep.h>  // AVR library for controlling the sleep modes

/////screen
//#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C
// Define proper RST_PIN if required.
#define RST_PIN -1
SSD1306AsciiAvrI2c oled;


/////pHmeter
#define Offset 1.00            //pH deviation compensate
#define samplingInterval 30
#define ArrayLenth  40    //times of collection
#define ph_Pin A2            //pH meter Analog output to Arduino Analog Input 0

////sensors e mix stuff
#define dtrPin 7
#define sleepPin 8
#define OledPin 9
#define ECHOPIN 4// Pin to receive echo pulse
#define TRIGPIN 5// Pin to send trigger pulse
#define TdsSensorPin A1

#define alarmPin 2 // The number of the pin for monitoring alarm status on DS3231

/////variables

int timer = 0;
int  rtc_hour;

RTC_DS3231 rtc;

static String buff;
const unsigned long timeout = 30000;

String inData;
String battData;
String Buffer;

int Volt;

static float pHValue, voltage;
float tdsValue = 0;
int pHVal;
int Dist;
float Temperature = 0;
float Temp_E = 0;
float TurbVolt;
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex = 0;


void(* resetFunc) (void) = 0; //dichiarazione funzione reset



void setup() {

  Serial.begin(9600);
  gprsSerial.begin(9600);  //Begin serial communication with SIM800
  sensors.begin();

  pinMode(alarmPin, INPUT_PULLUP); // Set alarm pin as pullup
  pinMode(dtrPin, OUTPUT);    // sets the digital as output
  pinMode(sleepPin, OUTPUT);    // sets the digital as output
  pinMode(OledPin, OUTPUT);    // sets the digital as output
  pinMode(ECHOPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(ECHOPIN, HIGH);
  digitalWrite(OledPin, HIGH);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(dtrPin, LOW);

  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  // If required set time
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // To compiled time
  //rtc.adjust(DateTime(2020, 7, 3, 20, 0, 0)); // Or explicitly, e.g. July 3, 2020 at 8pm

  // Disable and clear both alarms
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  rtc.writeSqwPinMode(DS3231_OFF); // Place SQW pin into alarm interrupt mode
   rtc.disable32K();  //we don't need the 32K Pin, so disable it




  // print current time
  char date[10] = "hh:mm:ss";
  rtc.now().toString(date);
  Serial.println(date);





  delay(100);
#if RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
#endif // RST_PIN >= 0
  // Call oled.setI2cClock(frequency) to change from the default frequency.
  oled.setFont(Verdana12_bold);
  oled.clear();
  oled.set2X();
  oled.print("BilaSens6//l22");
  delay(1000);
  oled.clear();
  oled.print(date);

  Serial.print("pre-waking SIM800...");
  digitalWrite(dtrPin, LOW);
  gprsSerial.println("AT");
  delay(1000);
  gprsSerial.println("AT+CSCLK=0");
  Serial.println("GET BATT SIM800...");
  waitForResponse();
  getBatteryLevel();



  ///////////////////////////////first rec in case of test/reboot
  Serial.println("BilaSens5//m22");
  Temp_E = rtc.getTemperature();
  delay(200);
  TempSens();
  delay(200);
  EchoDist();
  delay(200);
  TurbSens();
  delay(200);
  TDSSens();
  delay(200);
  phMeter();
  delay(200);

  Intvalmtr();
  digitalWrite(sleepPin, LOW);

  oleddrawchar();      // Draw characters of the default font


 // Gsm ();  //for test GSM on boot

}


//////OLEDdisp//////

void oleddrawchar(void) {
  oled.setFont(Adafruit5x7);
  oled.set1X();
  oled.clear();
  oled.print("ph:");
  oled.println(pHValue);
  oled.print("temp:");
  oled.print(Temperature);
  oled.println("C");
  oled.print("tds:");
  oled.print(tdsValue);
  oled.println("ppm");
  oled.print("dist:");
  oled.print(Dist);
  oled.print("cm ");
  oled.print(Volt);
  oled.print("mV");
  delay(6000);
}




/////////////////////////////// GSM transmission and string concat

void Gsm () {


  // stopSLeep
  Serial.print("pre-waking SIM800...");

  digitalWrite(dtrPin, LOW);
  gprsSerial.println("AT");
  delay(1000);
  gprsSerial.println("AT+CSCLK=0");
  Serial.println("waking SIM800...");
  waitForResponse();
  getBatteryLevel();


  request_url = THING_SPEAK_API_URL;
  request_url.concat("&field1=");
  request_url.concat(Dist);
  request_url.concat("&field2=");
  request_url.concat(TurbVolt);
  request_url.concat("&field3=");
  request_url.concat(Temperature);
  request_url.concat("&field4=");
  request_url.concat(pHValue);
  request_url.concat("&field5=");
  request_url.concat(tdsValue);
  request_url.concat("&field6=");
  request_url.concat(Volt);
  request_url.concat("&field7=");
  request_url.concat(Temp_E);

  gprsSerial.println("AT");
  waitForResponse();

  gprsSerial.println("AT+CPIN?");
  waitForResponse();

  gprsSerial.println("AT+CREG?");
  waitForResponse();

  gprsSerial.println("AT+CGATT?");
  waitForResponse();

  gprsSerial.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  waitForResponse();

  gprsSerial.println("AT+CIPSHUT");
  waitForResponse();

  gprsSerial.println("AT+CIPSTATUS");
  waitForResponse();

  gprsSerial.println("AT+CIPMUX=0");
  waitForResponse();

  gprsSerial.println("AT+CSTT=\"iot.1nce.net\"");//start task and setting the APN,
  waitForResponse();

  gprsSerial.println("AT+CIICR");//bring up wireless connection
  waitForResponse();

  gprsSerial.println("AT+CIFSR");//get local IP adress
  ShowSerialData();
  delay(3000);

  gprsSerial.println("AT+CIPSPRT=0");
  delay(3000);
  ShowSerialData();

  gprsSerial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection
  delay(6000);
  ShowSerialData();

  gprsSerial.println("AT+CIPSEND");//begin send data to remote server
  delay(4000);
  ShowSerialData();

  gprsSerial.println(request_url);//begin send data to remote server
  delay(4000);
  ShowSerialData();

  gprsSerial.println((char)26);//sending
  delay(5000);//waitting for reply, important! the time is base on the condition of internet
  gprsSerial.println();
  ShowSerialData();

  gprsSerial.println("AT+CIPSHUT");//close the connection
  delay(100);
  ShowSerialData();


  // start SLeep
  //  gprsSerial.println("AT + CFUN = 0");
  //  ShowSerialData();

  digitalWrite(dtrPin, HIGH);
  delay(50);
  gprsSerial.println("AT+CSCLK=2");
  waitForResponse();
  delay(5000);
}
///////////////////////////////end transmission

/////////////////////////////// GSM related stuff
void ShowSerialData()
{
  while (gprsSerial.available() != 0)
    Serial.write(gprsSerial.read());
  //  oled.clear();
  //  oled.println(gprsSerial.read());
  delay(500);
}

static void waitForResponse() {
  if (wait()) {
    buff = gprsSerial.readString();
    Serial.print(buff);
    //    oled.clear();
    //    oled.print(buff);

  }
  else {
    buff = "";
  }
}

static boolean wait()
{
  unsigned long start = millis();
  unsigned long delta = 0;

  while (!gprsSerial.available()) {
    delta = millis() - start;
    if (delta >= timeout) {
      Serial.println("no risp!");
      oled.setFont(Adafruit5x7);
      oled.set1X();
      oled.clear();
      oled.print("wait for solar batt charge ");
      delay(2000);
      oled.ssd1306WriteCmd(SSD1306_DISPLAYOFF); // To switch display off

      digitalWrite(sleepPin, LOW);

      //go to sleep 6 hour
      sleep_enable();                       // Enabling sleep mode
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Setting the sleep mode, in this case full sleep

      noInterrupts();                       // Disable interrupts
      attachInterrupt(digitalPinToInterrupt(alarmPin), alarm_ISR, LOW);

      Serial.println("Going to sleep!");    // Print message to serial monitor
      Serial.flush();                       // Ensure all characters are sent to the serial monitor

      interrupts();                         // Allow interrupts again
      sleep_cpu();                          // Enter sleep mode
      /* The program will continue from here when it wakes */

      // Disable and clear alarm
      // rtc.disableAlarm(1);
      rtc.clearAlarm(1);

      Serial.println("I'm back!");          // Print message to show we're back
      break;
    }
  }
  return gprsSerial.available();
}

void getBatteryLevel() {
  Volt = 0;
  battData = "";
  gprsSerial.println("AT+CBC"); // battery level
  delay(500);
  while (gprsSerial.available()) {
    battData += char(gprsSerial.read());
  }
  Volt = battData.substring(20, 24).toInt();
  delay(500);
}


void enterSleep() {

  //sleep
  digitalWrite(LED_BUILTIN, LOW);
  oled.ssd1306WriteCmd(SSD1306_DISPLAYOFF); // To switch display off
  //digitalWrite(OledPin, LOW);

  // GSM
  Serial.print("preparing sleep SIM800...");

  digitalWrite(dtrPin, LOW);
  gprsSerial.println("AT");
  delay(1000);
  gprsSerial.println("AT+CSCLK=0");
  waitForResponse();

  digitalWrite(dtrPin, HIGH);
  delay(10);
  gprsSerial.println("AT+CSCLK=2");
  waitForResponse();
  delay(1000);


  //cpu
  sleep_enable();                       // Enabling sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Setting the sleep mode, in this case full sleep

  noInterrupts();                       // Disable interrupts
  attachInterrupt(digitalPinToInterrupt(alarmPin), alarm_ISR, LOW);

  Serial.println("Going to sleep!");    // Print message to serial monitor
  Serial.flush();                       // Ensure all characters are sent to the serial monitor

  interrupts();                         // Allow interrupts again
  sleep_cpu();                          // Enter sleep mode

  /* The program will continue from here when it wakes */

  // Disable and clear alarm
  //rtc.disableAlarm(1);
  rtc.clearAlarm(1);

  Serial.println("I'm back!");          // Print message to show we're back
}


void alarm_ISR() {
  // This runs when SQW pin is low. It will wake up the µController
  sleep_disable(); // Disable sleep mode
  detachInterrupt(digitalPinToInterrupt(alarmPin)); // Detach the interrupt to stop it firing
}



void loop() {
  DateTime now = rtc.now();  // Get current time


  if (timer == 1) {
    rtc.setAlarm1(rtc.now() + TimeSpan(0, 6, 0, 0), DS3231_A1_Hour);
    //rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, 0, 0), DS3231_A1_Minute);
    //rtc.setAlarm1(rtc.now() + TimeSpan(59), DS3231_A1_Second);

    digitalWrite(sleepPin, HIGH);
    delay(4000);
    Temp_E = rtc.getTemperature();
    delay(200);
    TempSens();
    delay(200);
    EchoDist();
    delay(200);
    TurbSens();
    delay(200);
    TDSSens();
    delay(200);
    phMeter();
    delay(200);

    //    Intvalmtr();
    digitalWrite(sleepPin, LOW);
    Gsm();
    enterSleep();  // Go to sleep

  }

  if (timer == 0) {

    rtc_hour =  now.hour();
    Serial.println("rtc_hour from rtc");
    Serial.println(rtc_hour);

    oled.setFont(Adafruit5x7);
    oled.set1X();
    oled.clear();

    if (rtc_hour < 12) {
      rtc.setAlarm1(DateTime(0, 0, 0, 12, 0, 0), DS3231_A1_Hour); // Set first alarm
      timer = 1;
      oled.print("next alarm: 12");
      delay(2000);
      enterSleep();
    }

    if (rtc_hour > 12 && rtc_hour < 18) {
      rtc.setAlarm1(DateTime(0, 0, 0, 18, 0, 0), DS3231_A1_Hour); // Set first alarm
      oled.print("next alarm: 18");
      delay(2000);
      timer = 1;
      enterSleep();
    }

    if (rtc_hour > 18) {
      rtc.setAlarm1(DateTime(0, 0, 0, 0, 0, 0), DS3231_A1_Hour); // Set first alarm
      oled.print("next alarm: 00");
      delay(2000);
      timer = 1;
      enterSleep();
    }

  }
}

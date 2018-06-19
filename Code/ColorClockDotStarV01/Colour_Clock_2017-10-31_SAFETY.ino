char CodeFilename[] = "Colour_Clock 2017-10-31 (SAFETY)";


#include <FastLED.h>
#include <DS3231.h>
#include <Wire.h>    //This library allows you to communicate with I2C / TWI devices. 
#include <ACE128.h>  // Absolute Contact Encoder
#include <ACE128map12345678.h> // mapping for pin order 12345678
#include "Adafruit_FRAM_I2C.h"

#define NUM_LEDS 303// Number of LEDs in strip

//#define DATA_PIN    11 //Uno
//#define CLOCK_PIN   13 //Uno

#define DATA_PIN    11 //Nano
#define CLOCK_PIN   13 //Nano
#define LED_TYPE  APA102

ACE128 myACE((uint8_t)0, (uint8_t*)encoderMap_12345678); // datasheet basics
CRGB leds[NUM_LEDS];
CRGB ledHold[1];

//RTC Variables
DS3231 Clock;
bool Century = false;
bool h12;
bool PM;
bool ADy, A12h, Apm;
byte year, month, date, DoW, hour, minute, second;

int RTChour, RTCmins, RTCsecs = 0;  // Time: HH,MM,SS
int RTCyear, RTCmnth, RTCdate;      // Date: YY,MM,DD  <---year is two digits!
int RTCtemp;                        // Temperature

byte ADay, AHour, AMinute, ASecond, ABits;

long HOUR24=0;
long HOUR12=0;
int  MINS =0;
int  SECS = 0;
int  SECS_B4 = 0;
long YER = 0;
int  MTH = 0;
int  DAT = 0;

int TotalHues = 256;
uint8_t HueShift = 0; //Deviation of specturm from above layout. Values: 0  to (TotalColours-1)

int TimeHue;


//int RGB[3];  
byte HueValues[13]; //Hue values array

//HSI Variables (H range: 0..360 S&I range: 0..1)
double Hue = 0;
int Saturation =255;
int Intensity  =150;
int IntensityCentre  =100;
int IndicatorLEDLength =4;

byte Centre = 0;
int LED01 = 200;
int LED02 = 202;
int LED03 = 204;
int LED04 = 206;
int LED05 = 208;
int LED06 = 210;
int LED07 = 212;
int LED08 = 214;
int LED09 = 192;
int LED10 = 194;
int LED11 = 196;
int LED12 = 198;

int LedNumber[13] = {
  0,
  255,
  259,
  263,
  267,
  271,
  275,
  279,
  283,
  287,
  291,
  295,
  299};

int CENTRE_S = 0;  //216; //Centre LED numbers
int CENTRE_F = 254;

uint32_t ColourValue;
Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();
uint16_t          framAddr = 0;


byte KnobIndex=0;      // Table index values from MCP23008 (greycode)
byte KnobPhysicalPosition = 0;      // MCP23008 values converted into postion value
int  KnobVirtualPosition = 0;
int  KnobPositionChange = 0; 
byte KnobZeroPosition = 0;       // Zero postion of Knob (adjustable)



// --- HSL -----------------
int maxPWM= 255; // DotStar PWM per colour 0...255

void setup() {

  Wire.begin();         // Start the I2C interface
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  LedsAllOff();
  fram.begin();         //Start FRAM non-volatile memory module
  Serial.begin(115200); // Start the serial interface
  attachInterrupt(0, ButtonA, LOW);  //Interrupt 0 = Pin2
  attachInterrupt(1, ButtonB, LOW);  //Interrupt 1 = Pin3
         // 3 second delay for recovery 
  delay(50);           // half second delay for recovery 

  myACE.begin();    // this is required for each instance, initializes the IO expander
  KnobIndex=myACE.acePins(); 
  KnobPhysicalPosition=myACE.rawPos();        // get raw mechanical position (must have KnobIndex for this to work)
  KnobVirtualPosition = fram.read8(0x2);
  KnobVirtualPosition = (fram.read8(0x3))+KnobVirtualPosition;
  myACE.setZero(KnobPhysicalPosition); // sets logical zero position  remember where we are
  KnobZeroPosition=myACE.getZero();
  KnobPositionChange=myACE.pos();            // get logical position (signed): -64 -> +63 


  Serial.print("Code Filename: ");Serial.println(CodeFilename);
  Serial.println("- CLOCK ON - ");
  
  //Serial.print("KnobIndex: ");Serial.println(KnobIndex);
  Serial.print("*Knob Physical: ");Serial.print(KnobPhysicalPosition);
  Serial.print(" Zero: ");Serial.print(KnobZeroPosition);
  Serial.print(" Change: ");Serial.print(KnobPositionChange);
  Serial.print(" Virtual: ");Serial.println(KnobVirtualPosition );


  //Clock.setHour(16);
  //Clock.setMinute(07);   //Hard Time set.
  //Clock.setSecond(00);

  
  HueShift = fram.read8(0x1);
  ReadRTC();
  TimeIndexHueConversion();
  ShiftTimeHue();
  SetIndicators();
  SetCentre();
}

void ButtonA()
{
 static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 200ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 200) 
 {
  Serial.println("Button A");
  IntensityCentre=IntensityCentre+10;
  if (IntensityCentre > 255)
      IntensityCentre=0;
  Serial.print("Intensity Centre: ");Serial.println(IntensityCentre);
  SetCentre();
 }
 last_interrupt_time = interrupt_time;
 delay(500);
}


 
void ButtonB()
{
 static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 200ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 200) 
 {
  Serial.println("Button B");
  Intensity=Intensity+10;
  if (Intensity > 255)
      Intensity=0;
  Serial.print("Intensity :");Serial.println(Intensity);
SetIndicators();
 }
 last_interrupt_time = interrupt_time;
 delay(500);
}


void FramBreakout() {

  if (fram.begin()) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
    Serial.println("Found I2C FRAM");
  } else {
    Serial.println("I2C FRAM not identified ... check your connections?\r\n");
    Serial.println("Will continue in case this processor doesn't support repeated start\r\n");
  }
  
  // Read the first byte
  uint8_t test = fram.read8(0x0);
  Serial.print("Restarted "); Serial.print(test); Serial.println(" times");
  // Test write ++
  fram.write8(0x0, test+1);
}


void loop() {
  SECS_B4 = RTCsecs; //Records Current seconds to check for change after getting new time from RTC
  do {
    ReadRTC();
    CheckHueKnobPosition();   // Check knob posiotn & update HueShift

  }
  while (!Secs_Different());  // Loops until RTC secs change
  PrintRTCTime();             //Print new RTC time
  TimeIndexHueConversion();
  ShiftTimeHue();
  SetCentre();
      Serial.print(" Knob Physical: ");Serial.print(KnobPhysicalPosition);
      Serial.print(" Zero: ");Serial.print(KnobZeroPosition);
      Serial.print(" Change: ");Serial.print(KnobPositionChange);
      Serial.print(" Virtual: ");Serial.println(KnobVirtualPosition );
}



// ----------------------------------------------
void LedsAllOff(){
  for (int LEDCENTRE = CENTRE_S; LEDCENTRE < (CENTRE_F+1); LEDCENTRE++)
    leds[LEDCENTRE] =CRGB::Black; //CentreOff
    FastLED.show();  // Refresh Centre
    
  for (int Indicator = 1; Indicator < 13; Indicator++){
    for (int LEDINDICATOR = LedNumber[Indicator]; LEDINDICATOR < (LedNumber[Indicator]+IndicatorLEDLength); LEDINDICATOR++)
      leds[LEDINDICATOR] =CRGB::Black; //IndicatorsOff
      FastLED.show();   // Refresh Indicators
  }
}
// ----------------------------------------------
void ReadRTC() {
  RTCsecs = Clock.getSecond();
  RTCmins = Clock.getMinute();
  RTChour = Clock.getHour(h12, PM);
  RTCdate = Clock.getDate();
  RTCmnth = Clock.getMonth(Century);
  RTCyear = Clock.getYear();
  RTCtemp = Clock.getTemperature();
  HOUR24 = RTChour;
  HOUR12 = HOUR24;
  if (HOUR12 > 11)
    HOUR12 = HOUR12 - 12;
  MINS = RTCmins;
  SECS = RTCsecs;
}

// ----------------------------------------------
boolean Secs_Different() {
  boolean SecsDifferent = false;
  if (RTCsecs != SECS_B4) {
    SecsDifferent = true;
    return SecsDifferent;
  }
  else {
    return SecsDifferent;
  }
}
// ----------------------------------------------
void TimeIndexHueConversion(){
  long MaxTimeIndex = 43200;
  long TimeIndex = (HOUR12*60*60)+(MINS*60)+SECS;  //Convert time into 12hr index value (Max: 43200)
  Serial.print(" TimeIndex:");Serial.print(TimeIndex);

  TimeHue = map (TimeIndex , 0, (MaxTimeIndex-1), 0, (TotalHues-1));  //map 43200 positions into colour range 
  Serial.print(" TimeHue: ");
  padding(TimeHue,3);
  //Serial.print(); 
}


// ----------------------------------------------
void SetCentre(){
  HueValues[Centre]=TimeHue;
  for (int LEDCENTRE = (CENTRE_S); LEDCENTRE < (CENTRE_F+1); LEDCENTRE++)
    leds[LEDCENTRE].setHSV(HueValues[Centre], Saturation, IntensityCentre);
  FastLED.show();   // Refresh strip
  delay(20);        // Pause 20 milliseconds (~50 FPS)

}

// ----------------------------------------------
void ShiftTimeHue(){  //Add hue shift to TimeIndex
  Serial.print(" HueShift: "); padding(HueShift,3); Serial.print("  "); 
  
  TimeHue = TimeHue + HueShift;

  if (TimeHue > (TotalHues-1))
    TimeHue = TimeHue - TotalHues;

  Serial.print(" Total Shift: "); Serial.println(TimeHue);

}


// ----------------------------------------------
void CheckHueKnobPosition(){          // Check for dial turn & update Hueshift
    KnobIndex=myACE.acePins();       // Read IO expander pin values from MCP23008
    KnobPhysicalPosition=myACE.rawPos();        // get raw mechanical position (must have pinPos for this to work)
    KnobZeroPosition=myACE.getZero();
    KnobPositionChange=myACE.pos();            // get logical position (signed): -64 -> +63 


    if (KnobPositionChange != 0 ) {    // Check Knob movement
      
      KnobVirtualPosition = KnobVirtualPosition  + KnobPositionChange; // Increase/Decrease value by amount dial has changed.
      if (KnobVirtualPosition  > 511)                                  //Roll-over control
        KnobVirtualPosition  = KnobVirtualPosition -512; 
      if (KnobVirtualPosition  < 0)
        KnobVirtualPosition  = KnobVirtualPosition +512;               //Roll-under control

      myACE.setZero(KnobPhysicalPosition); // sets logical zero position  remember where we are

      if (KnobVirtualPosition > 255){
         fram.write8(0x2, 255); 
         fram.write8(0x3, (KnobVirtualPosition+1));}
      else {
         fram.write8(0x2, KnobVirtualPosition);
         fram.write8(0x3, 0);}
          

      HueShift = map (KnobVirtualPosition , 0, 511, 0, (TotalHues-1));  //map 512 positions into colour range 
      fram.write8(0x1, HueShift);

      //Serial.print("KnobIndex: ");Serial.println(KnobIndex);
      PrintRTCTime();
      TimeIndexHueConversion();
      ShiftTimeHue(); 
      Serial.print(">Knob Physical: ");Serial.print(KnobPhysicalPosition);
      Serial.print(" Zero: ");Serial.print(KnobZeroPosition);
      Serial.print(" Change: ");Serial.print(KnobPositionChange);
      Serial.print(" Virtual: ");Serial.println(KnobVirtualPosition );
       // Serial.print("Hue Shift: ");Serial.println(HueShift);
      //Serial.println("________________________________");
      //Serial.println();*/

      
      SetIndicators();
      TimeIndexHueConversion();
      ShiftTimeHue();
      SetCentre();
      }
}


// ----------------------------------------------
void FadeCentreIn(){
  for (int Index = 0; Index < (Intensity+1); Index++){
    for (int LEDCENTRE = CENTRE_S; LEDCENTRE < (CENTRE_F+1); LEDCENTRE++)
      leds[LEDCENTRE].setHSV(HueValues[Centre], Saturation, Index);
    FastLED.show();
    delay(7);
    }
}
// ----------------------------------------------
void FadeCentreOut(){
  for (int Index = Intensity; Index > -1; Index--){
    for (int LEDCENTRE = CENTRE_S; LEDCENTRE < (CENTRE_F+1); LEDCENTRE++)
      leds[LEDCENTRE].setHSV(HueValues[Centre], Saturation, Index);
    FastLED.show();
    delay(7);
    }
}
// ----------------------------------------------
void SetIndicators(){

int IndicatorHue = 0; //Value for Indicator 12. Others can be determined with this.
float IndicatorDifference = TotalHues / 12;  // Spectrum Index difference between indicators 3600

 for (int Indicator = 1; Indicator < 13; Indicator++){     //Loops for Indicator = 12...1

    IndicatorHue = ((Indicator)* IndicatorDifference)+ HueShift;  //Cals index value for each indicator
    if (IndicatorHue > (TotalHues-1))  
      IndicatorHue = IndicatorHue - TotalHues;  //Roll-over

    HueValues[Indicator]=IndicatorHue;
    for (int LedtoSet = LedNumber[Indicator]; LedtoSet < (LedNumber[Indicator]+IndicatorLEDLength); LedtoSet++) //Cycles through all Leds within Indicator. 
        leds[LedtoSet].setHSV(HueValues[Indicator], Saturation, Intensity); //Sets Led values

    FastLED.show();   // Refresh strip
    //PrintIndicatorHues(Indicator);
    //delay(100);        // Pause 20 milliseconds (~50 FPS)
    }
}

// ----------------------------------------------
void BlinkIndicator(int Indicator){ //Blink Led
  int BlinkAmount = 100;
  delay(100);
  for (int LedtoSet = LedNumber[Indicator]; LedtoSet < (LedNumber[Indicator]+IndicatorLEDLength); LedtoSet++) //Cycles through all Leds within Indicator. 
    leds[LedtoSet] = CHSV(0, 0, 0); //Sets Led Off
    //leds[(LedNumber[LedIndex])] = CHSV(0, 0, 0);
  FastLED.show();
  delay(100);
  for (int LedtoSet = LedNumber[Indicator]; LedtoSet < (LedNumber[Indicator]+IndicatorLEDLength); LedtoSet++) //Cycles through all Leds within Indicator. 
     leds[LedtoSet].setHSV(HueValues[Indicator], Saturation, Intensity); //Sets Led values 
     //leds[(LedNumber[LedIndex])].setHSV((HueValues[LedIndex]), Saturation, Intensity); 



   /*for (int LEDINDICATOR = LedNumber[Indicator]; LEDINDICATOR < (LedNumber[Indicator]+IndicatorLEDLength); LEDINDICATOR++) //Cycles through all Leds within Indicator. 
     leds[LEDINDICATOR].setHSV(HueValues[Indicator], Saturation, Intensity); //Sets Led values*/
}


// ----------------------------------------------
void BlinkLed(int LedIndex){ //Blink Individual Led : 0 - NUMLEDS
  int BlinkAmount = 100;
  delay(150);
   leds[(LedNumber[LedIndex])] = CHSV(0, 0, 0);
  FastLED.show();
  delay(150);
  leds[(LedNumber[LedIndex])].setHSV((HueValues[LedIndex]), Saturation, Intensity); 

}


// ----------------------------------------------
void PrintRTCTime() {
  //Serial.println("________________________");
  //Serial.print("20");
  //Serial.print(RTCyear);
  //Serial.print('-');
  //padding(RTCmnth,2);
  //Serial.print('-');
  //padding(RTCdate,2);
  //Serial.print(' ');
  padding(RTChour,2);
  Serial.print(':');
  padding(RTCmins,2);
  Serial.print(':');
  padding(RTCsecs,2);
  Serial.print(" ");
  //Serial.print(",");
  //padding(RTCtemp,2);
  //Serial.write(176);  // = ° Degree symbol
  //Serial.println("C");
}
// ----------------------------------------------
void PrintIndicatorHues(int Indicator) {
    Serial.print("Indicator ");
    padding(Indicator,2);
    Serial.print(": "); 
    Serial.print("Hue=");
    padding(HueValues[Indicator],3);
    Serial.println();
} 

// ----------------------------------------------
void CentreOff(){
  for (int LEDCENTRE = CENTRE_S; LEDCENTRE < (CENTRE_F+1); LEDCENTRE++)
    leds[LEDCENTRE] =CRGB::Black;
    FastLED.show(); 
}


// ----------------------------------------------
void padding(long number, byte pad ) {
 int currentMax = 10;
 for (byte i=1; i<pad; i++){
   if (number < currentMax) {
     Serial.print("0");
   }
   currentMax *= 10;
 } 
 Serial.print(number);
}


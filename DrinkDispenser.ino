/* Drink Mixer with scale 

 * library design: Weihong Guan (@aguegu)
 * http://aguegu.net
 *
 * library host on
 * https://github.com/aguegu/Arduino
 */

// Hx711.DOUT - pin #A1
// Hx711.SCK - pin #A0
#include <Adafruit_NeoPixel.h>  //library for the neopixel strip
#include <Hx711.h>              //library for the scale
Hx711 scale(A1, A0);
#define N_LEDS 6


//Port definition
const int PIN = 3;          // Data pin for the Neopixel LEDs
const int latchPin = 4;     // Latch pins are now connected
const int LEDclockPin = 5;  // Clocking for the Display LED shift register
const int LEDdataPin = 6;   // data for the Display LED shift register
const int clockInPin = 7;   // Clocking for the push button shift register
const int dataInPin = 8;    // Data for the push button shift register
const int clockOutPin = 9;  // Clocking for the pump shift register
const int dataOutPin = 10;  // Data for the pump shift register
const int modePin = A2;     // Switch the mode from manual to automatic
const int startPin = A3;    // The Button which starts didpensing or starts cleaning procedure
const int armPin = A4;      // This in enables the Mosfet Shift register if set low


// initializing the system
float Weight = 0;
float NewWeight = 0;
float Fill = 0;
int pixelMask = 0;
int Selection = 0;
int LastSelection = 0;
int Mode = 0;
int Drink = 0;
int InputMask = 0;
int LastInputMask = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);
uint32_t Black = strip.Color(0, 0, 0);
uint32_t Red = strip.Color(255, 0, 0);
uint32_t Green = strip.Color(0, 255, 0);
uint32_t Blue = strip.Color(10, 10, 255);
uint32_t Yellow = strip.Color(255, 255, 0);
uint32_t Magenta = strip.Color(255, 0, 255);
uint32_t White = strip.Color(155, 255, 255);
uint32_t Orange = strip.Color(255, 153, 0);

// Defining the values in an array, maybe later this will be stored on a card
// Changed so that Drink 1 is a Long Island and the rest are single drinks
String Drinks[][9] = {
  { "Content", "Long Island", "Vodka", "Rum", "Gin", "Tequilla", "Triple Sec", "Lime Juice", "Whiskey" },
  { "Long Island", "0", "10", "10", "10", "10", "10", "10", "0" },
  { "Vodka", "0", "20", "20", "0", "0", "0", "0", "0" },
  { "Gin", "0", "0", "20", "0", "0", "0", "0", "0" },
  { "Rum", "0", "0", "0", "20", "0", "0", "0", "0" },
  { "Tequilla", "0", "0", "0", "0", "20", "0", "0", "0" },
  { "Triple Sec", "0", "0", "0", "0", "0", "20", "0", "0" },
  { "Lime Juice", "0", "0", "0", "0", "0", "0", "20", "0" },
  { "Whiskey", "10", "0", "0", "0", "0", "0", "0", "0" },


};

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'

  pinMode(LEDclockPin, OUTPUT);
  pinMode(LEDdataPin, OUTPUT);
  pinMode(clockInPin, OUTPUT);
  pinMode(dataInPin, INPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockOutPin, OUTPUT);
  pinMode(dataOutPin, OUTPUT);
  pinMode(armPin, OUTPUT);
  pinMode(modePin, INPUT);
  pinMode(startPin, INPUT);

  // initialize the output for the Motors
  digitalWrite(latchPin, LOW);
  shiftOut(dataOutPin, clockOutPin, MSBFIRST, B00000000);
  digitalWrite(latchPin, HIGH);
  delay(10);
  //up from now the motors are active
  digitalWrite(armPin, LOW);
  setDisplay(0);  // Initialize Display Shift Register
}

void loop() {

  if ((digitalRead(modePin) == HIGH)) {  //Manual mode
    Serial.println("Manuell");
    setDisplay(0);
    //check if a glass is on the scale
    if (scale.getGram() < 100) {  //no glas
      setLeds(255, Red);
    } else {  //glas on
      Selection = getSelection();
      if (Selection != 0) {
        pixelMask = Selection;
        setLeds(pixelMask, Blue);
        setPump(Selection);
      } else {
        setLeds(255, Green);
        setPump(0);
      }
    }

    delay(10);
  } else {
    Serial.println("Auto");       // Automatic mode
    if (scale.getGram() < 100) {  //no glas
      setLeds(255, Red);
    } else {  // glas on

      if (Selection == 0) {    //Testing for a valid selection
        setLeds(255, Yellow);  //Nothing Selected, almost ready
      } else {
        setLeds(255, Green);  // Drink selected
      }
      InputMask = getSelection();  //reading the pushbuttons
      if (InputMask != LastInputMask) {
        if (InputMask != 0) {
          Selection = InputMask;
        }
      }
      LastInputMask = InputMask;
      setDisplay(Selection);
    }
    delay(1);  // delay in between reads for stability
    Serial.print("Selection: ");
    Serial.println(Selection);
    Serial.println(scale.getGram());
    if ((digitalRead(startPin) == HIGH) & (Selection != 0)) {
      mixDrink(Selection);
      Serial.println("Mixing");
      Serial.println("Done!");
      checkRemoval();
      delay(1000);
    }
  }
}
//Extract the neccesary ingrediences
void mixDrink(int Selection) {

  switch (Selection) {
    case 1:
      Drink = 1;
      break;
    case 2:
      Drink = 2;
      break;
    case 4:
      Drink = 3;
      break;
    case 8:
      Drink = 4;
      break;
    case 16:
      Drink = 5;
      break;
    case 32:
      Drink = 6;
      break;
    case 64:
      Drink = 7;
      break;
    case 128:
      Drink = 8;
      break;
  }

  Serial.println("Mixing a " + Drinks[Drink][0]);
  Serial.println("----------------------------------------------");
  for (int i = 1; i <= 8; i++) {
    // check if the ingredience is used
    if (Drinks[Drink][i] != "0") {
      poorIngrediannce(Drink, i);
    }
  }
}
//Dispense the ingrediance
void poorIngrediannce(int Selection, int Valve) {
  int Pump;
  //converting the string of the Array to a float
  char floatbuf[8];
  Drinks[Selection][Valve].toCharArray(floatbuf, sizeof(floatbuf));
  float Amount = atof(floatbuf);

  // Determination of the stop criteria
  Weight = (scale.getGram());
  NewWeight = (Weight + Amount);

  Serial.print("New weight is: ");
  Serial.println(NewWeight);
  Serial.print("Weight is: ");
  Serial.println(Weight);
  Serial.print("adding " + Drinks[Selection][Valve] + " cl of " + Drinks[0][Valve]);
  Serial.println("");
  // detect the pump
  switch (Valve) {
    case 1:
      Pump = B00000001;
      Serial.println("Starting Pump 1");
      break;
    case 2:
      Pump = B00000010;
      Serial.println("Starting Pump 2");
      break;
    case 3:
      Pump = B00000100;
      Serial.println("Starting Pump 3");
      break;
    case 4:
      Pump = B00001000;
      Serial.println("Starting Pump 4");
      break;
    case 5:
      Pump = B00010000;
      Serial.println("Starting Pump 5");
      break;
    case 6:
      Pump = B00100000;
      Serial.println("Starting Pump 6");
      break;
    case 7:
      Pump = B01000000;
      Serial.println("Starting Pump 7");
      break;
    case 8:
      Pump = B00100000;
      Serial.println("Starting Pump 8");
      break;
  }
  setLeds(Pump, White);  // start pump
  Serial.println(Pump);
  digitalWrite(latchPin, LOW);
  shiftOut(dataOutPin, clockOutPin, MSBFIRST, Pump);
  digitalWrite(latchPin, HIGH);
  delay(10);


  //wait for the Glass to fill up,Timer routine for bottle empty needs to go in here
  while (Weight < NewWeight) {
    Weight = (scale.getGram());
  }
  Serial.println("Max reached");
  setLeds(Pump, Black);  // stop pump
  digitalWrite(latchPin, LOW);
  shiftOut(dataOutPin, clockOutPin, MSBFIRST, B0000000);
  digitalWrite(latchPin, HIGH);
  delay(10);
}


void setPump(int number) {  // Function to start or stop the motors
  digitalWrite(latchPin, LOW);
  shiftOut(dataOutPin, clockOutPin, MSBFIRST, number);
  digitalWrite(latchPin, HIGH);
  Serial.println(number, BIN);
}


int getSelection() {  //Funktion to read out the pushbutton Shift register
  int Button;
  digitalWrite(latchPin, LOW);
  delayMicroseconds(10);
  digitalWrite(clockInPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(latchPin, HIGH);
  Button = shiftIn(dataInPin, clockInPin, MSBFIRST);
  delayMicroseconds(10);
  digitalWrite(latchPin, LOW);
  Serial.print("Input Mask: ");
  Serial.println(Button);
  return Button;
}
int setDisplay(int Mask) {  //Funktion to set out the Display LEDS
  Serial.print("Output Mask: ");
  Serial.println(Mask);
  digitalWrite(latchPin, LOW);
  delayMicroseconds(10);
  shiftOut(LEDdataPin, LEDclockPin, MSBFIRST, Mask);
  digitalWrite(latchPin, HIGH);
  delay(10);
}

void setLeds(int Mask, uint32_t Color) {  //Funktion to set out the Bottle LEDs
  for (int i = 0; i < 6; i++) {
    if (bitRead(Mask, i)) {
      strip.setPixelColor(i, Color);
    } else {
      strip.setPixelColor(i, Black);
    }
  }
  strip.show();
}

void checkRemoval() {  //Function to check the removal of the filled glass

  while ((scale.getGram() > 100)) {

    for (int i = 1; i < 64; i = i * 2) {  //Bilk unti Glass is removd
      setLeds(i, Green);
      delay(80);
      setLeds(i, Black);
      delay(100);
    }
  }
  Selection = 0;
  NewWeight = 0;
  setDisplay(0);
}

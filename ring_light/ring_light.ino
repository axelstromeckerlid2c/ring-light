/*
 File: ring_light.ino
 Author: Axel Ström Eckerlid
 Date: 2024-02-28
 Det här programmet kombinerar en klocka och en termometer med hjälp av en RTC-modul 
 och en ultraljudssensor för att antingen visa temperaturen som en färggradient eller justera 
 ljusstyrkan på en Adafruit Neopixel ring light baserat på närhet.
*/


//Inkluderar bibliotek
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <RtcDS3231.h>


//Initierar RTC modul
RtcDS3231<TwoWire> Rtc(Wire);
float temp;


//piezo pin
#define beep 4

--------------------------------------------------------------------------------------------
// Ultrasonisk Sensor
const int ECHO_PIN = 7;
const int TRIG_PIN = 6; 

const int max_distance = 20; //cm
const int min_distance = 1.9;
float duration_us, distance_cm;
--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------
//Ring light
#define LIGHT_PIN  2
#define LED_COUNT 24

Adafruit_NeoPixel ring(LED_COUNT, LIGHT_PIN, NEO_RGB + NEO_KHZ800);

int r = 255;
int g;
int b;

String light_mode = "brightness";
int brightness; //Ring light ljusstyrka (0-255)
String current_color = "RED";

//Färgtemperatur skala (1 - 1000)
float value;
float minimum = 1;
float maximum = 1000;
--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------
//Knapp
const int BUTTON_PIN = 11;
int hold_ms = 1000; //Tid för långt knapptryck(millisekunder)
unsigned long press_duration = 0; //Tilldelas millis() i void loop
--------------------------------------------------------------------------------------------

void setup() {
  Serial.begin (9600);
  Wire.begin();

  //Bestämmer PINMODE
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  //Setup för ring light
  ring.begin();
  ring.show();
  ring.setBrightness(255);
}

void loop() {
  //Kallar på funktioner
  us_sensor();
  updateRTC();

  brightness = map(distance_cm, min_distance, max_distance, 1, 255); //Beräknar brightness på skala från distans

  //Då knappen är nedtryckt stannar resten av programmet (för att sluta uppdatera "press_duration").
  press_duration = millis();
  while ((digitalRead(BUTTON_PIN) == 1)) {
  }

  //kallar på funktioner
  toggle_button();
  light_show();
  buzzer();

  ring.show(); //Uppdaterar ring light

  delay(50);
}


/*
  Beräknar tid för knapptryck och byter läge/färg på ring light
  returnerar: void
*/
void toggle_button() {
  if (((millis() - press_duration) >= hold_ms) && (light_mode == "heat")) {
    light_mode = "brightness";
  }
  else if (((millis() - press_duration) >= hold_ms) && (light_mode == "brightness")) {
    light_mode = "heat";
  }

  if (((millis() - press_duration) <= hold_ms) && ((millis() - press_duration) >= 50) && (light_mode == "brightness")) {
    change_color();
    delay (50);
  }
}

/*
  Bestämmer ljusstyrka samt färg på ring light (efter "light_mode", "distance_cm" eller "temp").
  returnerar: void
*/
void light_show() {
  if (light_mode == "brightness") {
    if ((distance_cm < max_distance) && (distance_cm != 0)) {
      light_set();
    }
    else {
      light_off();
    }
  }
  else if (light_mode == "heat") {
    if ((temp < 15) && (temp > 0)) {
      value = map(temp, 0.0, 15.0, 1, 70);
    }
    else if (temp > 15) {
      value = map(temp, 15, 60, 530, 700);
    }
    else { //Då "temp" < 0
      value = 1;
    }

    convert_to_rgb(minimum, maximum, value, r, g, b); //Kallar på funktion för att bestämma r, g och b för färgtemperatur

    for (int i = 0; i < LED_COUNT; i++) {
      ring.setPixelColor(i, g, r, b);
    }
  }
}

/*
  Ändrar färg på ring light
  returnerar: void
*/
void change_color() {
  r = 0;
  g = 0;
  b = 0;
  if (current_color == "GREEN") {
    b = (255);
    current_color = "BLUE";
    delay (200);
  }
  else if (current_color == "RED") {
    g = (255);
    current_color = "GREEN";
    delay (200);
  }
  else if (current_color == "BLUE") {
    r = (255);
    current_color = "RED";
    delay (200);
  }
}


/*
  Sätter färg och ljusstyrka på ring light
  returnerar: void
*/
void light_set() {
  for (int i = 0; i < LED_COUNT; i++) {
    ring.setPixelColor(i, (g / brightness), (r / brightness), (b / brightness));
  }
}

/*
  Släcker ljuset på ring light
  returnerar: void
*/
void light_off() {
  for (int i = 0; i < LED_COUNT; i++) {
    ring.setPixelColor(i, 0, 0, 0);
  }
}


/*
  Kör ultrasonisk sensor och beräknar "distance_cm".
  returnerar: void
*/
void us_sensor() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // measure duration of pulse from ECHO pin
  duration_us = pulseIn(ECHO_PIN, HIGH);
  // calculate the distance
  distance_cm = ((duration_us * 0.0343) / 2);
}


/*
  Beräknar ring light färg efter temperatur
  Parametrar:
    - float minimum
    - float maximum
    - float value
    - int r
    - int g
    - int b
  returnerar: void
*/
void convert_to_rgb(float minimum, float maximum, float value, int &r, int &g, int &b) {
  minimum = float(minimum);
  maximum = float(maximum);
  float halfmax = ((minimum + maximum) / 2);

  if (minimum <= value && value <= halfmax) {
    r = 0;
    g = int(255. / (halfmax - minimum) * (value - minimum));
    b = int(255. + -255. / (halfmax - minimum) * (value - minimum));
  }
  else if (halfmax < value && value <= maximum) {
    r = int(255. / (maximum - halfmax) * (value - halfmax));
    g = int(255. + -255. / (maximum - halfmax) * (value - halfmax));
    b = 0;
  }
}

/*
  Hämtar temperatur från RTC-modulen
  returnerar: void
*/
void updateRTC() {
  //Hämtar temperaturen från rtc-modulen.
  RtcTemperature rtcTemp = Rtc.GetTemperature();
  temp = rtcTemp.AsFloatDegC();
}

/*
  Piezoelement skickar ljudsignal av olika intensitet beroende på "distance_cm".
  returnerar: void
*/
void buzzer() {
    if ((distance_cm < max_distance) && (distance_cm != 0)) {
    tone(beep, (2000/distance_cm));
    delay(distance_cm * 6);
    noTone(beep);
  }
}

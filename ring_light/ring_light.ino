/*
 File: Klocktermometer.ino
 Author: Axel Ström Eckerlid
 Date: 2024-02-13
  Vad gör programmet?
*/


//Inkluderar bibliotek
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <RtcDS3231.h>


//Initierar RTC modul
RtcDS3231<TwoWire> Rtc(Wire);
float temp;

// Ultrasonisk Sensor
const int ECHO_PIN = 7;
const int TRIG_PIN = 6; 

//(cm)
const int max_distance = 20;
const int min_distance = 1.9;

float duration_us, distance_cm;


//Ring light
#define LIGHT_PIN  2
#define LED_COUNT 24

Adafruit_NeoPixel ring(LED_COUNT, LIGHT_PIN, NEO_RGB + NEO_KHZ800);

String light_mode = "brightness";
int brightness; //Ring light ljusstyrka (0-255)
String current_color = "RED";

int r = 255;
int g;
int b;

//Färgtemperatur skala (1 - 1000)
float value;
float minimum = 1;
float maximum = 1000;


//Knappen
const int BUTTON_PIN = 11;
int hold_ms = 1000; //Long press button time (milliseconds)

unsigned long press_duration = 0; //Knapp nedtryckt räknare


//piezo pin
#define beep 4



void setup() {
  Serial.begin (9600);

  //Bestämmer PINMODE
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  //Setup för ring light
  ring.begin();
  ring.show();
  ring.setBrightness(255);

  Wire.begin();
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

  ring.show(); //Uppdaterar ring light

  buzzer();

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
    else {
      value = 1;
    }

    convert_to_rgb(minimum, maximum, value, r, g, b);

    for (int i = 0; i < LED_COUNT; i++) {
      ring.setPixelColor(i, g, r, b);
    }
  }
}

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
  Kör ultrasonisk sensor och beräknar distans
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

void buzzer() {
    if ((distance_cm < max_distance) && (distance_cm != 0)) {
    tone(beep, (2000/distance_cm));
    delay(distance_cm * 6);
    noTone(beep);
  }
}

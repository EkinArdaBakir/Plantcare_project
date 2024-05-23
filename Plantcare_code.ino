#define BLYNK_TEMPLATE_ID "UW TEMPLATE ID"
#define BLYNK_TEMPLATE_NAME "Plantcare"
#define BLYNK_AUTH_TOKEN "UW AUTHORISATION TOKEN"

#include <BlynkSimpleEsp8266.h>
#include "Adafruit_seesaw.h"
#include <DHT11.h>
#include "HX711.h"

#include <ESP8266WiFi.h>


HX711 scale;
Adafruit_seesaw ss;
DHT11 dht11(D0);



char ssid[] = "UW WIFI NAAM";
char pass[] = "UW WIFI WACHTWOORD";


uint8_t datares = D4;
uint8_t clockres = D5;
uint8_t pomp = D8;

int vochtigheidgewenst = 60;
unsigned long previousMillis = 0;
const unsigned long interval = 30000;  // 30 seconds

int tellercalibratie = 0;
bool calibratie = false;
BlynkTimer timer;
int waardedroog = 990;
int waardenat = 1200;
uint16_t capread = 0;
int capreadwaarde = 0;

BLYNK_WRITE(V2) {
  vochtigheidgewenst = param.asInt();
}

void setup() {

  Serial.begin(9600);

  scale.begin(datares, clockres);
  pinMode(pomp, OUTPUT);
  ss.begin(0x36);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(7200000L, myTimerEvent);
}

void loop() {
  Serial.println("pre blynk");
  long blynk_start = millis();
  Blynk.run();
  Serial.print("post blynk: ");
  Serial.println(millis() - blynk_start);
  timer.run();
}

bool pumpActive = false;
unsigned long pumpStartTime = 0;

void myTimerEvent() {

  // Read sensor values
  capread = ss.touchRead(0);
  capreadwaarde = map(capread, waardedroog, waardenat, 0, 100);
  int temperature = dht11.readTemperature();
  Serial.println("test");
  // Update Blynk
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V3, capreadwaarde);

  static bool pumpActive = false;

  // Control pump

  if (capreadwaarde < (vochtigheidgewenst - 10)) {
    Serial.println("test2");
    float res = scale.get_units(10);
    Blynk.virtualWrite(V4, res);
    if (res > 15) {
      Serial.println("test3");
      digitalWrite(pomp, HIGH);
      delay(3000);
      digitalWrite(pomp, LOW);
    }
  }
}



BLYNK_WRITE(V0) {
  static enum { EMPTYING,
                FILLING,
                CALIBRATED } state = EMPTYING;
  static unsigned long stateStartTime = millis();
  const unsigned long emptyingDuration = 5000;
  const unsigned long fillingDuration = 5000;

  switch (state) {
    case EMPTYING:
      Serial.println("Leeg het reservoir");
      if (millis() - stateStartTime >= emptyingDuration) {
        scale.tare();
        state = FILLING;
        stateStartTime = millis();
      }
      break;

    case FILLING:
      Serial.println("Vul het reservoir");
      if (millis() - stateStartTime >= fillingDuration) {
        scale.calibrate_scale(100, 5);
        state = CALIBRATED;
        stateStartTime = millis();
      }
      break;

    case CALIBRATED:
      Serial.println("Kalibratie klaar");
      calibratie = false;  // Calibration finished
      state = EMPTYING;
      break;
  }
}

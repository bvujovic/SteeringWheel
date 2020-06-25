// Rad sa GY na osnovu: raw.ino primera iz Examples foldera biblioteke MechaQMC5883
// Po potrebi ispitati nesto drugaciji nacin konektovanja na server: https://www.arduino.cc/en/Reference/WiFiClient

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <MechaQMC5883.h>
#include <WiFiServerBasics.h>

MechaQMC5883 qmc;
WiFiClient client;
char serverIP[] = "192.168.0.50"; // IP servera na vozilu
const int serverPort = 80;

enum Phase
{
  Calibrating,
  Testing,
  Connecting,
  Driving,
  Pause,
};
Phase phase;

const ulong itvCal = 1000; // trajanje kalibracije (u ms)
int msCal;                 // millis() vrednost kada pocne kalibracija

const int itvGY = 500;      // na koliko ms se uzimaju vrednosti sa GY senzora
const int pinBtn = D7;      // taster za pause/drive
const int pinMinSpeed = D5; // prekidac kojim se bira izmedju promenljive (0) i minimalne (1) brzine vozila
const int pinLed = LED_BUILTIN;

uint16_t gyX, gyY, gyZ; // podaci dobijeni od gyro senzora
int dx, dy, dz;         // pomeraji po osama dobijeni na osnovu kalibracije
int x, y, z;            // kalibrisani podaci: sirovi sa gyro senzora + pomeraji
int i = 0;

int cnt = 0;

void blinking(int n, int itv)
{
  for (int i = 0; i < n - 1; i++)
  {
    digitalWrite(pinLed, false);
    delay(itv);
    digitalWrite(pinLed, true);
    delay(itv);
  }
  digitalWrite(pinLed, false);
  delay(itv);
  digitalWrite(pinLed, true);
}

void statusLedON(bool on) { digitalWrite(pinLed, !on); }

int pitch, roll;
// Racunanje pitch i roll u stepenima na osnovu sirovih gyro podataka
void CalcDegrees()
{
  const int maxAngle = 30;

  // calc pitch
  const int minX = 75;
  const int maxX = 450;

  if (x > -minX && x < minX)
    pitch = 0;
  else
  {
    pitch = constrain(x, -maxX, maxX);
    int sign = pitch > 0 ? +1 : -1;
    pitch = map(abs(pitch), minX, maxX, 0, maxAngle) * sign;
  }
  // Serial.println(pitch);

  // calc roll
  const int maxRoll = 500;  // 30°
  const int minRoll = -350; // -30°
  roll = constrain(y, minRoll, maxRoll);
  if (roll > 0)
    roll /= (500.0 / maxAngle);
  else
    roll /= (350.0 / maxAngle);

  // Serial.println(pitch);
  // Serial.println(roll);
}

float cmdX, cmdY;
// /act?x=0.33&y=-0.5&t=1000&f=1
String CreateVehicleCmd()
{
  // pitch -> y (smer i brzina vozila)
  cmdY = pitch == 0 ? 0 : map(pitch, -30, 30, 100, -100) / 100.0;
  // roll -> x (pravac)
  cmdX = map(roll, -30, 30, -100, 100) / 100.0;

  return String("GET /act?x=") + cmdX + "&y=" + cmdY + "&t=1000&f=1";
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  qmc.init();

  pinMode(pinBtn, INPUT_PULLUP);
  pinMode(pinMinSpeed, INPUT);
  pinMode(pinLed, OUTPUT);
  phase = Connecting;
}

void loop()
{
  if (phase == Connecting)
  {
    statusLedON(true);
    ConnectToWiFi();
    // Serial.println(WiFi.status() == WL_CONNECTED);

    // konektovanje na server vozila
    if (!client.connect(serverIP, serverPort))
      blinking(3, 250);
    else
      blinking(1, 500);

    phase = Calibrating;
  }

  if (phase == Calibrating)
  {
    statusLedON(true);
    static int i = 0;
    static int xSum = 0, ySum = 0;

    if (i++ == 0)
      msCal = millis();

    qmc.read(&gyX, &gyY, &gyZ);
    xSum += (short)gyX;
    ySum += (short)gyY;

    if (millis() < itvCal + msCal)
      delay(20);
    else
    {
      dx = xSum / i;
      dy = ySum / i;

      // Serial.print(dx);
      // Serial.print('\t');
      // Serial.println(dy);
      // Serial.println();

      phase = Driving;
      // phase = Testing;
      statusLedON(false);
    }
  }

  if (phase == Testing)
  {
    qmc.read(&gyX, &gyY, &gyZ);
    x = (short)gyX - dx;
    y = (short)gyY - dy;

    // Prikaz kalibrisanih podataka sa gyro senzora
    // Serial.print(x);
    // Serial.print('\t');
    // Serial.println(y);

    CalcDegrees();
    CreateVehicleCmd();

    delay(itvGY);
  }

  if (phase == Driving)
  {
    if (!client.connected())
      client.connect(serverIP, serverPort);

    qmc.read(&gyX, &gyY, &gyZ);
    x = (short)gyX - dx;
    y = (short)gyY - dy;
    CalcDegrees();
    String cmd = CreateVehicleCmd() + " HTTP/1.1\r\n" +
                 "Host: " + serverIP + "\r\n" +
                 "Connection: close\r\n";
    // Serial.println(cmd);
    client.println(cmd);
    client.stop();

    if (!digitalRead(pinBtn))
    {
      phase = Pause;
      statusLedON(true);
    }

    delay(itvGY);
  }

  if (phase == Pause)
  {
    if (!digitalRead(pinBtn))
    {
      phase = Driving;
      statusLedON(false);
    }
    delay(itvGY);
  }
}

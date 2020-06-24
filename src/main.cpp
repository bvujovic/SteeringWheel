// Rad sa GY na osnovu: raw.ino primera iz Examples foldera biblioteke MechaQMC5883
// Po potrebi ispitati nesto drugaciji nacin konektovanja na server: https://www.arduino.cc/en/Reference/WiFiClient

const bool DEBUG = false;

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <MechaQMC5883.h>

MechaQMC5883 qmc;
WiFiClient client;
char serverIP[] = DEBUG ? "192.168.0.14" : "192.168.0.10";
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

const int itvGY = 500;               // na koliko ms se uzimaju vrednosti sa GY senzora
const int itvTasterPoslednji = 3000; // koliko se ms ceka od poslednjeg klika na taster do konektovanja na server
int msTasterPoslednji = 0;           // millis() kada je taster pritisnut poslednji put
int cntTaster = 0;                   // koliko je puta taster pritisnut
bool valTaster = false;              // da li je taster pritisnut
const int pinTaster = D7;
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
  pitch = constrain(x, -450, 450); // 30° .. -30°
  pitch /= -15;                    // 450/30, a minus je zato sto je x<0 -> uspon/nos gore/pitch up

  const int maxRoll = 500;  // 30°
  const int minRoll = -350; // -30°
  roll = constrain(y, minRoll, maxRoll);
  if (roll > 0)
    roll /= (500.0 / 30);
  else
    roll /= (350.0 / 30);

  // Serial.println(pitch);
  // Serial.println(roll);
}

float cmdX, cmdY;
// /act?x=0.33&y=-0.5&t=1500&f=0
void CreateVehicleCmd()
{
  // pitch -> y (smer i brzina vozila)
  cmdY = map(pitch, -30, 30, 100, -100) / 100.0;

  // roll -> x (pravac)
  cmdX = map(roll, -30, 30, -100, 100) / 100.0;

  Serial.print(cmdY);
  Serial.print('\t');
  Serial.println(cmdX);
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  qmc.init();

  pinMode(pinTaster, INPUT);
  pinMode(pinMinSpeed, INPUT);
  pinMode(pinLed, OUTPUT);
  phase = Calibrating;
  Serial.println("Calibration...");
}

void loop()
{
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

      phase = Testing;
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

  if (phase == Connecting)
  {
    // konektovanje na WiFi
    const char *ssid = "Vujovic";
    const char *password = "......";
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.print("Volan IP: ");
    Serial.println(WiFi.localIP());

    // konektovanje na server vozila
    if (!client.connect(serverIP, serverPort))
      blinking(3, 250);
    else
      blinking(1, 500);

    phase = Driving;
  }

  if (phase == Driving)
  {
    const int maxXForward = 600;
    const int maxXBackward = -800;
    const int maxV = 240; // max brzina vozila
    const int minV = 70;  // min brzina za koju se vozilo krece; ispod te brzine salje se Stop (X) komanda
    int v;                // brzina vozila
    char dir;             // pravac: F/R/B/L

    qmc.read(&gyX, &gyY, &gyZ);
    x = (short)gyX - dx;
    y = (short)gyY - dy;
    // z = (short)gyZ - dz;
    if (x >= 0)
      v = maxV * (x >= maxXForward ? 1 : (float)x / maxXForward);
    else
      v = maxV * (x <= maxXBackward ? 1 : (float)x / maxXBackward);
    dir = x >= 0 ? 'F' : 'B';
    if (v < minV)
      dir = 'X';
    else
      v = minV + (v - minV) / 2; // ogranicenje brzine na max 150
    //T Serial.print("v: ");  Serial.println(v);

    //T Serial.println(millis() / 1000.0);
    if (!client.connected())
      client.connect(serverIP, serverPort);

    String cmd = DEBUG ? String("GET /volan_test/act.php?dir=") : String("GET /act?dir=");
    if (dir != 'X')
    {
      y = y > 0 ? 0.62 * y : 1.0 * y;
      y = constrain(y, -300, 300) / 20;
      if (dir == 'B')
        y = -y;
      //T Serial.print("dir: "); Serial.println(y);
      if (y < 0)
        y = 100 + abs(y);

      int isMinV = digitalRead(pinMinSpeed);
      //B Serial.println(isMinV);
      if (isMinV)
        v = minV;

      //B String cmd = String("GET /act?dir=") + dir + "&v=" + v + "&dir2=DC&v2=" + y + " HTTP/1.1\r\n" +
      cmd = cmd + dir + "&v=" + v + "&dir2=DC&v2=" + y + " HTTP/1.1\r\n" +
            "Host: " + serverIP + "\r\n" +
            "Connection: close\r\n";
      //T Serial.println(cmd);
      client.println(cmd);
    }
    else // zaustavljanje vozila
    {
      //B String cmd = String("GET /act?dir=X HTTP/1.1\r\n") + "Host: " + serverIP + "\r\n" + "Connection: close\r\n";
      cmd = cmd + String("X HTTP/1.1\r\n") + "Host: " + serverIP + "\r\n" + "Connection: close\r\n";
      //T Serial.println(cmd);
      client.println(cmd);
    }
    client.stop();

    if (digitalRead(pinTaster))
    {
      phase = Pause;
      digitalWrite(pinLed, false);
    }

    delay(itvGY);
  }

  if (phase == Pause)
  {
    if (digitalRead(pinTaster))
    {
      phase = Driving;
      digitalWrite(pinLed, true);
    }
    delay(itvGY);
  }
}

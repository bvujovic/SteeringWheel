// https://stackoverflow.com/questions/62224747/esp8266-433mhz-and-radiohead-library

#include <WiFiServerBasics.h>

const int pinLed = LED_BUILTIN;
void ledON(bool on) { digitalWrite(pinLed, !on); }

#include "RadioRecv.h"
RadioRecv radio;
ulong msRadioOff;

void setup()
{
    Serial.begin(115200);
    pinMode(pinLed, OUTPUT);
    ledON(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(100);
    radio.init();
}

void loop()
{
    if (radio.isON())
    {
        RadioRecvCode code = radio.refresh();
        if (code == WheelPos)
            radio.getMotCmd(); //* commands.add( radio.getmotcmd() );
        else if (code == End)
        {
            ledON(true);
            msRadioOff = millis();
        }
    }
    else
    {
        if (millis() - msRadioOff > 10000)
        {
            radio.resume();
            ledON(false);
        }
        delay(10);
    }
}

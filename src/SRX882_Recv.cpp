// https://stackoverflow.com/questions/62224747/esp8266-433mhz-and-radiohead-library

#include <RH_ASK.h>
#include <SPI.h>

RH_ASK driver(2000, 16, 12, 10, false); // ESP8266: do not use pin 11

void setup()
{
    Serial.begin(115200);
    if (!driver.init())
        Serial.println("init failed !!!");
}

int cnt = 0;
long ms = 0;
#define DATA_LEN 2
uint8_t n = DATA_LEN;
uint8_t s[DATA_LEN];

void loop()
{
    if (driver.recv(s, &n)) // Non-blocking
    {
        // driver.printBuffer("Got:", s, n);

        Serial.print((int8_t)s[0]);
        Serial.print('\t');
        Serial.println((int8_t)s[1]);
    }
}
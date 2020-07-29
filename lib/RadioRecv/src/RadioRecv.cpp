#include "RadioRecv.h"
#include "RadioRecvCode.h"

RadioRecv::RadioRecv()
{
    rh = new RH_ASK(2000, 16, 12, 10, false);
}

RadioRecv::~RadioRecv()
{
    Serial.println("~RadioRecv");
}

bool RadioRecv::init()
{
    radioOn = rh->init();
    Serial.println(radioOn ? "RadioRecv Started" : "RadioRecv Init Failed!");
    return radioOn;
}

void RadioRecv::end()
{
    radioOn = false;
    // Serial.print("RadioRecv Resume: ");
    // Serial.println(rh->mode());
    rh->sleep();
    // Serial.println(rh->mode());
}

void RadioRecv::resume()
{
    radioOn = true;
    // Serial.print("RadioRecv Resume: ");
    // Serial.println(rh->mode());
    rh->available();
    // Serial.println(rh->mode());
}

RadioRecvCode RadioRecv::refresh()
{
    if (rh->recv(s, &n)) // Non-blocking
    {
        // driver.printBuffer("Got:", s, n);
        int8_t x = s[0];
        int8_t y = s[1];
        // Serial.print(x);
        // Serial.print('\t');
        // Serial.println(y);

        if (y == 0 && (x == End || x == Pause))
        {
            if (x == End) // End kôd: prekid radio komunikacije
                end();
            return (RadioRecvCode)x;
        }
        return WheelPos;
    }
    else
        return None;
}

void RadioRecv::getMotCmd()
{
    const int8_t MAX_ANGLE = 45;
    const int8_t MIN_PITCH = 5;

    // pitch -> y (smer i brzina vozila)
    int8_t pitch = s[1];
    float cmdY;
    if (fabs(pitch) < MIN_PITCH)
        cmdY = 0;
    else if (pitch > 0)
        cmdY = map(pitch, MIN_PITCH, MAX_ANGLE, 0, 100) / 100.0F;
    else
        cmdY = map(pitch, -MAX_ANGLE, -MIN_PITCH, -100, 0) / 100.0F;

    // roll -> x (pravac)
    float cmdX = map((int8_t)s[0], -MAX_ANGLE, MAX_ANGLE, -100, 100) / 100.0F;

    // ako je cmdY == 0, a cmdX veoma malo -> vozilo se ne krece
    if (cmdY == 0 && fabs(cmdX) < 0.2)
        cmdX = 0;

    Serial.print(cmdX);
    Serial.print('\t');
    Serial.println(cmdY);
    //* return new WheelPos...
}
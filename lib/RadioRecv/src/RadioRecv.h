#pragma once

#include <RH_ASK.h>
#include <SPI.h>
#include <Arduino.h>
#include "Enums.h"

#define RADIO_DATA_LEN 2

class RadioRecv
{
private:
    RH_ASK *rh;
    uint8_t n = RADIO_DATA_LEN;
    uint8_t s[RADIO_DATA_LEN];
    bool radioOn = false;

public:
    RadioRecv();
    ~RadioRecv();

    bool init();
    RadioRecvCode refresh();
    void end();
    void resume();
    bool isON() { return radioOn; }
    void getMotCmd();
};

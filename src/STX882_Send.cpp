#include <SPI.h>
#include <RH_ASK.h>
RH_ASK rf;

const int pinLed = LED_BUILTIN;
#include "Joop.h"
Joop joop;

const int pinBtn = 9;
#include <ClickButton.h>
// Taster: pauza - 1 kratak klik, ponovna kalibracija - 2 kratka klika, kraj - 1 dugi klik
ClickButton btn(pinBtn, LOW, CLICKBTN_PULLUP);

enum Phase
{
    Calibrating,
    Driving,
    Pause,
    End,
    Failure
};
Phase phase;

const int itvSend = 500; // Vremenski interval (u ms) na koji se salju poruke volana ka vozilu.
const int DATA_LEN = 2;  // Duzina poruke (stringa) koju volan salje vozilu u bajtovima (karakterima).
uint8_t msg[DATA_LEN];   // Poruka volana vozilu. Uobicajeno: pitch i roll, eventualno: pauza ili kraj upravljanja.
long cntSend = 0;
long ms;

void ledON(bool on) { digitalWrite(pinLed, on); }

void resetCntSend() { cntSend = millis() / itvSend; }

void phaseChange(Phase newPhase)
{
    // Serial.print("Phase: ");
    // Serial.println(newPhase);
    phase = newPhase;
    if (phase == Calibrating || phase == End)
        ledON(true);
    if (phase == Driving)
    {
        ledON(false);
        resetCntSend();
    }
}

void phaseRefresh(unsigned long ms)
{
    if (phase == Pause)
        ledON(ms % 3000 < 300);
    if (phase == Failure)
        ledON(ms % 2000 < 1000);
}

void rfSend(int8_t x, int8_t y = 0)
{
    msg[0] = x;
    msg[1] = y;
    rf.send(msg, DATA_LEN);
    // rf.waitPacketSent();
    // 2 karaktera -> 78ms; 4 karaktera -> 90ms; 8 karaktera -> 114ms;

    // Serial.print(x);
    // Serial.print('\t');
    // Serial.println(y);
    // Serial.print(msg[0]);
    // Serial.print('\t');
    // Serial.println(msg[1]);
}

void setup()
{
    pinMode(pinLed, OUTPUT);
    phaseChange(Calibrating);
    rf.init();
    // Serial.begin(57600);
    if (joop.init())
        phaseChange(Driving);
    else
        phaseChange(Failure);
}

void loop()
{
    joop.refresh();

    if (phase == Driving && (ms = millis()) > cntSend * itvSend)
    {
        cntSend++;
        rfSend(joop.getRoll(), joop.getPitch());
    }

    phaseRefresh(millis());
    btn.Update();
    if (btn.clicks == 1) // 1 kratak klik: voznja <-> pauza
    {
        phaseChange(phase == Driving ? Pause : Driving);
        if (phase == Pause)
            rfSend(100 + phase);
        if (phase == Driving)
            resetCntSend();
    }
    if (btn.clicks == 2)
    {
        if (phase == Driving)
            ; //todo: slanje spec koda (Spin) koji ce vozilo razumeti kao obrtanje u mestu
    }
    if (btn.clicks == 3) // 3 kratka klika: ponovna kalibracija
    {
        if (phase == Driving)
            phaseChange(Pause);
        phaseChange(Calibrating);
        joop.calibrate();
        phaseChange(Driving);
        resetCntSend();
    }
    if (btn.clicks == -1) // 1 dugi klik: kraj
    {
        phaseChange(End);
        rfSend(100 + phase);
        //* sleep?
    }
}

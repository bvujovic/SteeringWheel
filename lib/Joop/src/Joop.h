#include <Arduino.h>
#include <Wire.h>

// Racunanje polozaja (pitch i roll) volana na osnovu podataka sa MPU-6050 senzora.
// http://www.brokking.net/imu.html
class Joop
{
private:
    //Declaring some global variables
    int16_t gyro_x, gyro_y, gyro_z;
    int32_t acc_x, acc_y, acc_z, acc_total_vector;
    // int temperature;
    int32_t gyro_x_cal, gyro_y_cal, gyro_z_cal;
    unsigned long loop_timer;
    int16_t lcd_loop_counter;
    float angle_pitch = 0.0, angle_roll = 0.0;
    float angle_pitch_prev, angle_roll_prev;
    int16_t angle_pitch_buffer, angle_roll_buffer;
    bool set_gyro_angles;
    float angle_roll_acc, angle_pitch_acc;
    float angle_pitch_output, angle_roll_output;

    const unsigned int HZ = 100;             // Ucestanost semplovanja. Originalno 250.
    const unsigned int US = 1000000 / HZ;    // Broj mikroseknudi po semplu.
    const float COEF1 = 1000.0 / HZ / 65500; // Joop: 0.0000611 = 1 / (250Hz / 65.5)
    const float COEF2 = COEF1 * PI / 180;    // Joop: 0.000001066 = 0.0000611 * (3.142(PI) / 180degr)
    const float COEF3 = 180 / PI;            // Joop: 57.296 = 1 / (3.142 / 180) The Arduino asin function is in radians

    void setup_mpu_6050_registers();
    bool read_mpu_6050_data();
    bool readSucc;
    int cntFails = 0;
    long cntOutput = 0;
    int i = 0;
    //B int pinLed; // Pin status LED-a.

public:
    //B Joop(int pinLed) { this->pinLed = pinLed; }

    bool calibrate();
    bool init();
    void refresh();
    float getPitch() { return -angle_pitch_output; }
    float getRoll() { return angle_roll_output; }
};

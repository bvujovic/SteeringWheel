#include "Joop.h"

void Joop::setup_mpu_6050_registers()
{
    //Activate the MPU-6050
    Wire.beginTransmission(0x68); //Start communicating with the MPU-6050
    Wire.write(0x6B);             //Send the requested starting register
    Wire.write(0x00);             //Set the requested starting register
    Wire.endTransmission();       //End the transmission
    //Configure the accelerometer (+/-8g)
    Wire.beginTransmission(0x68); //Start communicating with the MPU-6050
    Wire.write(0x1C);             //Send the requested starting register
    Wire.write(0x10);             //Set the requested starting register
    Wire.endTransmission();       //End the transmission
    //Configure the gyro (500dps full scale)
    Wire.beginTransmission(0x68); //Start communicating with the MPU-6050
    Wire.write(0x1B);             //Send the requested starting register
    Wire.write(0x08);             //Set the requested starting register
    Wire.endTransmission();       //End the transmission
}

bool Joop::calibrate()
{
    Serial.print("\nCalibrating gyro... ");
    const int n = 1000; // Joop: 2000
    for (int i = 0; i < n; i++)
    {
        if (i % 200 == 0)
            Serial.print('#');           //lcd.print(".");
        readSucc = read_mpu_6050_data(); //Read the raw acc and gyro data from the MPU-6050
        if (!readSucc)
            cntFails++;
        gyro_x_cal += gyro_x; //Add the gyro x-axis offset to the gyro_x_cal variable
        gyro_y_cal += gyro_y; //Add the gyro y-axis offset to the gyro_y_cal variable
        gyro_z_cal += gyro_z; //Add the gyro z-axis offset to the gyro_z_cal variable
        delay(3);             //Delay 3ms to simulate the 250Hz program loop
    }
    // Calc average offsets
    gyro_x_cal /= n;
    gyro_y_cal /= n;
    gyro_z_cal /= n;

    Serial.println();
    // Serial.println(gyro_x_cal);
    // Serial.println(gyro_y_cal);
    // Serial.println(gyro_z_cal);
    // Serial.println(" Done.");
    if (cntFails > 0)
    {
        Serial.print("Failed readings: ");
        Serial.println(cntFails);
        cntFails = 0;
        return false;
    }
    else
        return true;
}

bool Joop::init()
{
    Wire.begin();               //Start I2C as master
    setup_mpu_6050_registers(); //Setup the registers of the MPU-6050 (500dfs / +/-8g) and start the gyro
    //? delay(500);
    bool success = calibrate();
    loop_timer = micros(); //Reset the loop timer
    return success;
}

//B
// void Joop::write_LCD()
// {
//     if (cntOutput++ != 20)
//         return;
//     cntOutput = 0;

//     //Subroutine for writing the LCD
//     //To get a 250Hz program loop (4us) it's only possible to write one character per loop
//     //Writing multiple characters is taking to much time
//     if (lcd_loop_counter == 14)
//         lcd_loop_counter = 0; //Reset the counter after 14 characters
//     lcd_loop_counter++;       //Increase the counter
//     if (lcd_loop_counter == 1)
//     {
//         angle_pitch_buffer = angle_pitch_output * 10; //Buffer the pitch angle because it will change
//         // Serial.print(angle_pitch_buffer);
//         Serial.print(angle_pitch_output);
//         Serial.print('\t');
//     }

//     if (lcd_loop_counter == 8)
//     {
//         angle_roll_buffer = angle_roll_output * 10;
//         // Serial.println(angle_roll_buffer);
//         Serial.println(angle_roll_output);
//     }
// }

bool Joop::read_mpu_6050_data()
{                                 //Subroutine for reading the raw gyro and accelerometer data
    Wire.beginTransmission(0x68); //Start communicating with the MPU-6050
    Wire.write(0x3B);             //Send the requested starting register
    Wire.endTransmission();       //End the transmission
    Wire.requestFrom(0x68, 14);   //Request 14 bytes from the MPU-6050

    int i = 0;
    while (Wire.available() < 14) //Wait until all the bytes are received
        if (i++ > 10000)
        {
            // int av = Wire.available();
            // if (av != 0)
            //     Serial.println(av);
            return false;
        }

    // Serial.print("e");
    acc_x = Wire.read() << 8 | Wire.read(); //Add the low and high byte to the acc_x variable
    acc_y = Wire.read() << 8 | Wire.read(); //Add the low and high byte to the acc_y variable
    acc_z = Wire.read() << 8 | Wire.read(); //Add the low and high byte to the acc_z variable
    // temperature = Wire.read() << 8 | Wire.read(); //Add the low and high byte to the temperature variable
    Wire.read();
    Wire.read();
    gyro_x = Wire.read() << 8 | Wire.read(); //Add the low and high byte to the gyro_x variable
    gyro_y = Wire.read() << 8 | Wire.read(); //Add the low and high byte to the gyro_y variable
    gyro_z = Wire.read() << 8 | Wire.read(); //Add the low and high byte to the gyro_z variable

    // Serial.println(acc_x);
    // Serial.println(gyro_x);
    // Serial.println(gyro_y);
    // Serial.println(gyro_z);
    // while (true)
    //     delay(1000);
    return true;
}

void Joop::refresh()
{
    readSucc = read_mpu_6050_data(); //Read the raw acc and gyro data from the MPU-6050
    //! if (!readSucc && cntFails++ > 1000)
    if (!readSucc)
    {
        Serial.println('F');
        cntFails = 0;
    }
    // if (i++ % 1000 == 0)
    // {
    //     Serial.println(gyro_x);
    //     Serial.println(gyro_y);
    //     Serial.println(gyro_z);
    // }

    gyro_x -= gyro_x_cal; //Subtract the offset calibration value from the raw gyro_x value
    gyro_y -= gyro_y_cal; //Subtract the offset calibration value from the raw gyro_y value
    gyro_z -= gyro_z_cal; //Subtract the offset calibration value from the raw gyro_z value

    //Gyro angle calculations
    angle_pitch += (gyro_x * COEF1); //Calculate the traveled pitch angle and add this to the angle_pitch variable
    angle_roll += gyro_y * COEF1;    //Calculate the traveled roll angle and add this to the angle_roll variable
    // if (isnan(angle_pitch))
    // {
    //     angle_pitch = angle_pitch_prev;
    //     // Serial.println("$$$");
    //     // Serial.println(gyro_x);
    //     // Serial.println(gyro_y);
    //     // Serial.println(angle_roll);
    //     // Serial.println(gyro_x * COEF1);
    //     // while (true)
    //     //     delay(1000);
    // }
    // angle_pitch_prev = angle_pitch;
    // if (isnan(angle_roll))
    //     angle_roll = angle_roll_prev;
    // angle_roll_prev = angle_roll;

    // if (i++ % 1000 == 0)
    // {
    //     Serial.println(angle_pitch);
    //     Serial.println(angle_roll);
    // }

    angle_pitch += angle_roll * sin(gyro_z * COEF2); //If the IMU has yawed transfer the roll angle to the pitch angel
    angle_roll -= angle_pitch * sin(gyro_z * COEF2); //If the IMU has yawed transfer the pitch angle to the roll angel

    // Serial.println(angle_pitch);
    // Serial.println(angle_roll);
    // Serial.println('.');

    //Accelerometer angle calculations
    acc_total_vector = sqrt((acc_x * acc_x) + (acc_y * acc_y) + (acc_z * acc_z)); //Calculate the total accelerometer vector
    // Serial.println(acc_total_vector);

    angle_pitch_acc = asin((float)acc_y / acc_total_vector) * COEF3; //Calculate the pitch angle
    angle_roll_acc = asin((float)acc_x / acc_total_vector) * -COEF3; //Calculate the roll angle
    // Serial.println(angle_pitch_acc);
    // Serial.println(angle_roll_acc);
    // Serial.println('*');

    //Place the MPU-6050 spirit level and note the values in the following two lines for calibration
    angle_pitch_acc -= 0.0; //Accelerometer calibration value for pitch
    angle_roll_acc -= 0.0;  //Accelerometer calibration value for roll

    if (set_gyro_angles)
    {                                                                  //If the IMU is already started
        angle_pitch = angle_pitch * 0.9996 + angle_pitch_acc * 0.0004; //Correct the drift of the gyro pitch angle with the accelerometer pitch angle
        angle_roll = angle_roll * 0.9996 + angle_roll_acc * 0.0004;    //Correct the drift of the gyro roll angle with the accelerometer roll angle
    }
    else
    {                                  //At first start
        angle_pitch = angle_pitch_acc; //Set the gyro pitch angle equal to the accelerometer pitch angle
        angle_roll = angle_roll_acc;   //Set the gyro roll angle equal to the accelerometer roll angle
        set_gyro_angles = true;        //Set the IMU started flag
    }
    // Serial.println(angle_pitch);
    // Serial.println(angle_roll);

    //To dampen the pitch and roll angles a complementary filter is used
    angle_pitch_output = angle_pitch_output * 0.9 + angle_pitch * 0.1; //Take 90% of the output pitch value and add 10% of the raw pitch value
    angle_roll_output = angle_roll_output * 0.9 + angle_roll * 0.1;    //Take 90% of the output roll value and add 10% of the raw roll value

    // Serial.println(angle_pitch_output);
    // Serial.println(angle_roll_output);
    // Serial.println('%');
    // while (true)
    //     delay(1000);

    //B write_LCD(); //Write the roll and pitch values to the LCD display

    // if (usDiffBigCnt++ % 11 == 0)
    // {
    //     if (usDiffCnt < usDiffMax)
    //         usDiffs[usDiffCnt++] = micros() - loop_timer;
    //     else if (usDiffCnt == usDiffMax && !usDiffDisplay)
    //     {
    //         for (size_t i = 0; i < usDiffMax; i++)
    //         {
    //             Serial.print(usDiffs[i]);
    //             Serial.print('\t');
    //         }
    //         usDiffDisplay = true;
    //     }
    // }

    while (micros() - loop_timer < US)
        ;
    loop_timer = micros();
}

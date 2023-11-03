#include <Arduino.h>
#include <LedControl.h>

// *** LED Quadrant Pins *** //
#define GREEN_CH 18
#define BLUE_CH 19
#define RED_CH 5

#define Q1 23
#define Q2 22
#define Q3 1
#define Q4 3

// *** Pressure Sensor Pin *** //
#define PRESSURE_PIN 26

// *** Eye LedControl pins *** //
#define DIN_LEFT 17
#define CS_LEFT 16
#define CLK_LEFT 4

#define DIN_RIGHT 34
#define CS_RIGHT 35
#define CLK_RIGHT 32

uint counter = 0;

uint quadrants[] = {Q1, Q2, Q3, Q4};

int red_val = 255;
int green_val = 0;
int blue_val = 0;

long start_time = 0;

// *** Eye LedControl objects *** //
LedControl lc_left = LedControl(DIN_LEFT, CLK_LEFT, CS_LEFT, 1);
LedControl lc_right = LedControl(DIN_RIGHT, CLK_RIGHT, CS_RIGHT, 1);

byte anim_eye_blink[8][8] = {
    {B00000000, B00000000, B00000000, B01111110, B00000000, B00000000, B00000000, B00000000},
    {B00000000, B00000000, B00111100, B01000010, B00000000, B00000000, B00000000, B00000000},
    {B00000000, B00011000, B00100100, B01000010, B00000000, B00000000, B00000000, B00000000},
    {B00000000, B00000000, B00111100, B01000010, B00000000, B00000000, B00000000, B00000000},
    {B00000000, B00000000, B00000000, B01111110, B00000000, B00000000, B00000000, B00000000},
    {B00000000, B00000000, B00000000, B01000010, B00111100, B00000000, B00000000, B00000000},
    {B00000000, B00000000, B00000000, B01000010, B00100100, B00011000, B00000000, B00000000},
    {B00000000, B00000000, B00000000, B01000010, B00111100, B00000000, B00000000, B00000000}};

uint frame_counter = 0;

void setup()
{
    pinMode(PRESSURE_PIN, INPUT);

    ledcAttachPin(GREEN_CH, 0);
    ledcAttachPin(BLUE_CH, 1);
    ledcAttachPin(RED_CH, 2);

    ledcSetup(0, 4000, 8);
    ledcSetup(1, 4000, 8);
    ledcSetup(2, 4000, 8);

    pinMode(Q1, OUTPUT);
    pinMode(Q2, OUTPUT);
    pinMode(Q3, OUTPUT);
    pinMode(Q4, OUTPUT);

    start_time = millis();
    digitalWrite(Q1, HIGH);
    digitalWrite(Q2, LOW);
    digitalWrite(Q3, LOW);
    digitalWrite(Q4, LOW);

    // *** Eye LedControl setup *** //
    lc_left.shutdown(0, false);
    lc_left.setIntensity(0, 0);
    lc_left.clearDisplay(0);

    lc_right.shutdown(0, false);
    lc_right.setIntensity(0, 0);
    lc_right.clearDisplay(0);
}

void loop()
{
    // put your main code here, to run repeatedly:

    if (millis() - start_time > 30000)
    {
        for (int i = 0; i < 4; i++)
        {
            if (counter == i)
            {
                digitalWrite(quadrants[i], HIGH);
            }
            else
            {
                digitalWrite(quadrants[i], LOW);
            }
        }
        counter++;
        if (counter > 3)
        {
            counter = 0;
        }

        ledcWrite(0, random(0, 255));
        ledcWrite(1, random(0, 255));
        ledcWrite(2, random(0, 255));

        start_time = millis();
    }

    // *** Eye LedControl loop *** //
    if (millis() - start_time > 1000)
    {
        for (int i = 0; i < 8; i++)
        {
            lc_left.setRow(0, i, anim_eye_blink[frame_counter][i]);
            lc_right.setRow(0, i, anim_eye_blink[frame_counter][i]);
        }

        frame_counter++;
        if (frame_counter > 7)
        {
            frame_counter = 0;
        }
        start_time = millis();
    }

    // Randomly set different colors

    // int pressure = analogRead(PRESSURE_PIN);

    // if (pressure > 2000)
    // {
    //     Serial.println("Pressure detected");
    // }
    // else
    // {
    //     Serial.println("No pressure");
    // }
}

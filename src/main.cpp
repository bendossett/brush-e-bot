#include <Arduino.h>
#include <LedControl.h>
#include "anims.h"

// Uncomment this to get debug info in the serial monitor
// #define DEBUG

// *** Pressure Sensor Pin *** //
#define PRESSURE_PIN 0

// *** Eye LedControl pins *** //
#define DIN_LEFT 7
#define CS_LEFT 6
#define CLK_LEFT 5

long start_time = 0;

// *** Eye LedControl objects *** //
LedControl lc_left = LedControl(DIN_LEFT, CLK_LEFT, CS_LEFT, 2);

int frame_counter = 0;

const Anim *current_anim_left;
const Anim *current_anim_right;
int current_anim_duration = 0;
int current_anim_num_frames = 0;
long current_anim_start_time = 0;

const int NUM_PHASES = 13;

// All of the animations, in pairs of two, one for each eye
const Anim *const ANIM_LIST[26] = {
    &ANIM_OPEN_EYES,
    &ANIM_OPEN_EYES,
    &ANIM_WAIT_LEFT,
    &ANIM_WAIT_RIGHT,
    &ANIM_COUNTDOWN,
    &ANIM_COUNTDOWN,
    &ANIM_UPPER_LEFT_LEFT,
    &ANIM_UPPER_LEFT_RIGHT,
    &ANIM_COUNTDOWN,
    &ANIM_COUNTDOWN,
    &ANIM_UPPER_RIGHT_LEFT,
    &ANIM_UPPER_RIGHT_RIGHT,
    &ANIM_COUNTDOWN,
    &ANIM_COUNTDOWN,
    &ANIM_LOWER_LEFT_LEFT,
    &ANIM_LOWER_LEFT_RIGHT,
    &ANIM_COUNTDOWN,
    &ANIM_COUNTDOWN,
    &ANIM_LOWER_RIGHT_LEFT,
    &ANIM_LOWER_RIGHT_RIGHT,
    &ANIM_COUNTDOWN,
    &ANIM_COUNTDOWN,
    &ANIM_EXCITED_EYES,
    &ANIM_EXCITED_EYES,
    &ANIM_CLOSE_EYES,
    &ANIM_CLOSE_EYES};

// The duration that each animation should play for
//   0: play once
//   > 0: play for x number of seconds
//   < 0: take x seconds to play once
int ANIM_DURATION[13] = {
    0,
    10,
    -10,
    20,
    -10,
    20,
    -10,
    20,
    -10,
    20,
    -10,
    10,
    0,
};

// Number of frames in each animation in the sequence
int anim_frame_count[13] = {
    7,
    8,
    40,
    12,
    40,
    12,
    40,
    13,
    40,
    13,
    40,
    8,
    7,
};

// Track the status of each animation phase
//    0 = not complete
//    1 = complete
int phase_complete[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

// Current phase
int phase = 0;
// Current animation (2 per phase)
int anim_idx = 0;

// Did we just start a new phase?
bool new_phase = false;

// Did we just start running?
bool just_started = false;

int frame_step = 1;

// Setup runs once when the microcontroller first turns on
void setup()
{
    // Pressure pin is an input
    pinMode(PRESSURE_PIN, INPUT);

    start_time = millis();

    // Initialize the LED matrices
    for (int i = 0; i < lc_left.getDeviceCount(); i++)
    {
        lc_left.shutdown(i, false);
        lc_left.setIntensity(i, 0);
        lc_left.clearDisplay(i);
    }

#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("Starting");
#endif

    current_anim_duration = ANIM_DURATION[0];
    current_anim_left = ANIM_LIST[0];
    current_anim_right = ANIM_LIST[1];
    current_anim_start_time = millis();
    current_anim_num_frames = anim_frame_count[0];
}

void loop()
{
    // if pressure is above threshold don't do anything
    if (analogRead(PRESSURE_PIN) < 50)
    {
        return;
    }

    delay(250);

    // if phase is complete, move to next phase
    if (phase_complete[phase])
    {
        phase++;
        anim_idx += 2;

        // If that was the last phase, reset everything
        if (phase >= NUM_PHASES)
        {
            phase = 0;
            anim_idx = 0;

            for (int i = 0; i < NUM_PHASES; i++)
            {
                phase_complete[i] = 0;
            }
        }

        new_phase = true;

#ifdef DEBUG
        Serial.println("Starting phase " + String(phase));
#endif
    }

    if (new_phase)
    {
        new_phase = false;

        // Set up variables for this phase
        current_anim_duration = ANIM_DURATION[phase];
        current_anim_left = ANIM_LIST[anim_idx];
        current_anim_right = ANIM_LIST[anim_idx + 1];
        current_anim_start_time = millis();
        current_anim_num_frames = anim_frame_count[phase];

        frame_counter = 0;
    }

    if (current_anim_duration == 0)
    {
        // The anim should be played only once.
#ifdef DEBUG
        Serial.println("Frame counter: " + String(frame_counter) + " / " + String(current_anim_num_frames));
#endif
        // It's an 8x8 matrix, so loop through 8 rows and set all the 8 columns in that row
        //   Do this for both eyes (the first parameter of setRow specifies which eye)
        for (int i = 0; i < 8; i++)
        {
            lc_left.setRow(0, i, current_anim_left->anim[(frame_counter * 8) + i]);
            lc_left.setRow(1, i, current_anim_right->anim[(frame_counter * 8) + i]);
        }

        // Increment frame_counter
        if (frame_counter < current_anim_num_frames - 1)
        {
            frame_counter++;
        }
        else
        {
            phase_complete[phase] = 1;
        }
    }
    else if (current_anim_duration > 0)
    {
        // The anim should be looped for a certain duration

        long current_time = millis();

        // Have we been running for longer than the duration?
        if (current_time - current_anim_start_time > current_anim_duration * 1000)
        {
            phase_complete[phase] = 1;
        }
        else
        {
#ifdef DEBUG
            Serial.println("Frame counter: " + String(frame_counter) + " / " + String(current_anim_num_frames));

            // Print a message about how long we have been looping
            Serial.println("Current time: " + String(current_time - current_anim_start_time) + " / " + String(current_anim_duration * 100));
#endif
            for (int i = 0; i < 8; i++)
            {
                lc_left.setRow(0, i, current_anim_left->anim[(frame_counter * 8) + i]);
                lc_left.setRow(1, i, current_anim_right->anim[(frame_counter * 8) + i]);
            }

            // When we reach the end of the animation, reverse the direction that it's iterating
            //   and play the animation backwards
            if (frame_counter == current_anim_num_frames - 1)
            {
                frame_step = -1;
            }
            else if (frame_counter == 0)
            {
                frame_step = 1;
            }

            frame_counter += frame_step;
        }
    }
    else
    {
        // The anim should take a certain duration to play once

        long current_time = millis();

        if (current_time - current_anim_start_time > (-current_anim_duration * 1000))
        {
            phase_complete[phase] = 1;
        }
        else
        {
#ifdef DEBUG
            Serial.println("Frame counter: " + String(frame_counter) + " / " + String(current_anim_num_frames));
#endif
            for (int i = 0; i < 8; i++)
            {
                lc_left.setRow(0, i, current_anim_left->anim[(frame_counter * 8) + i]);
                lc_left.setRow(1, i, current_anim_right->anim[(frame_counter * 8) + i]);
            }

            // Should this actually be commented out?? 
            // Should probably double check that
            // delay((-current_anim_duration) * 100 / current_anim_num_frames);

            if (frame_counter < current_anim_num_frames - 1)
            {
                frame_counter++;
            }
            else
            {
                frame_counter = 0;
            }
        }
    }
}

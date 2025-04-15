#include <Arduino.h>
#include <LedControl.h>
#include "DFRobotDFPlayerMini.h"
#include "anims.h"
#include "driver/rtc_io.h"

// Uncomment this to get debug info in the serial monitor
// #define DEBUG
#define DEBUG_MUSIC

// *** Eye LedControl pins *** //
#define DIN_LEFT 23
#define CS_LEFT 5
#define CLK_LEFT 18

// *** DF Player Pins *** //
#define RXD2 16
#define TXD2 17

// *** Deep Sleep *** //
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)
#define WAKEUP_GPIO GPIO_NUM_26

long start_time = 0;

// *** Eye LedControl objects *** //
LedControl lc_left = LedControl(DIN_LEFT, CLK_LEFT, CS_LEFT, 2);

// *** DF Player Serial *** //
DFRobotDFPlayerMini music;

int frame_counter = 0;

const Anim *current_anim_left;
const Anim *current_anim_right;
int current_anim_duration = 0;
int current_anim_num_frames = 0;
long current_anim_start_time = 0;

const int NUM_PHASES = 12;

bool playing_eyes_close = false;

// All of the animations, in pairs of two, one for each eye
const Anim *const ANIM_LIST[24] = {
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
    &ANIM_EXCITED_EYES};

// The duration that each animation should play for
//   0: play once
//   > 0: play for x number of seconds
//   < 0: take x seconds to play once
int ANIM_DURATION[12] = {
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
};

// Number of frames in each animation in the sequence
int anim_frame_count[12] = {
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
};

// Track the status of each animation phase
//    0 = not complete
//    1 = complete
int phase_complete[12] = {
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
bool is_new_phase = false;

// Did we just start running?
bool just_started = false;

int frame_step = 1;

void printDetail(uint8_t type, int value)
{
    switch (type) {
      case TimeOut:
        Serial.println(F("Time Out!"));
        break;
      case WrongStack:
        Serial.println(F("Stack Wrong!"));
        break;
      case DFPlayerCardInserted:
        Serial.println(F("Card Inserted!"));
        break;
      case DFPlayerCardRemoved:
        Serial.println(F("Card Removed!"));
        break;
      case DFPlayerCardOnline:
        Serial.println(F("Card Online!"));
        break;
      case DFPlayerUSBInserted:
        Serial.println("USB Inserted!");
        break;
      case DFPlayerUSBRemoved:
        Serial.println("USB Removed!");
        break;
      case DFPlayerPlayFinished:
        Serial.print(F("Number:"));
        Serial.print(value);
        Serial.println(F(" Play Finished!"));
        break;
      case DFPlayerError:
        Serial.print(F("DFPlayerError:"));
        switch (value) {
          case Busy:
            Serial.println(F("Card not found"));
            break;
          case Sleeping:
            Serial.println(F("Sleeping"));
            break;
          case SerialWrongStack:
            Serial.println(F("Get Wrong Stack"));
            break;
          case CheckSumNotMatch:
            Serial.println(F("Check Sum Not Match"));
            break;
          case FileIndexOut:
            Serial.println(F("File Index Out of Bound"));
            break;
          case FileMismatch:
            Serial.println(F("Cannot Find File"));
            break;
          case Advertise:
            Serial.println(F("In Advertise"));
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

// Setup runs once when the microcontroller first turns on
void setup()
{
    esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(WAKEUP_GPIO), ESP_EXT1_WAKEUP_ANY_HIGH);
    /*
      If there are no external pull-up/downs, tie wakeup pins to inactive level with internal pull-up/downs via RTC IO
          during deepsleep. However, RTC IO relies on the RTC_PERIPH power domain. Keeping this power domain on will
          increase some power comsumption. However, if we turn off the RTC_PERIPH domain or if certain chips lack the RTC_PERIPH
          domain, we will use the HOLD feature to maintain the pull-up and pull-down on the pins during sleep.
    */
    rtc_gpio_pulldown_en(WAKEUP_GPIO);  // GPIO33 is tie to GND in order to wake up in HIGH
    rtc_gpio_pullup_dis(WAKEUP_GPIO);  

    pinMode(WAKEUP_GPIO, INPUT);

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

#ifdef DEBUG_MUSIC
    Serial.begin(115200);
    Serial.println("Starting");
#endif

    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    if (!music.begin(Serial2, true, true)) {  //Use serial to communicate with mp3.
      Serial.println(F("Unable to begin:"));
      Serial.println(F("1.Please recheck the connection!"));
      Serial.println(F("2.Please insert the SD card!"));
      while(true){
        delay(0); // Code to compatible with ESP8266 watch dog.
      }
    }
    Serial.println(F("DFPlayer Mini online."));

    current_anim_duration = ANIM_DURATION[0];
    current_anim_left = ANIM_LIST[0];
    current_anim_right = ANIM_LIST[1];
    current_anim_start_time = millis();
    current_anim_num_frames = anim_frame_count[0];
}

void loop()
{
    // if pressure is above threshold don't do anything
    if (analogRead(WAKEUP_GPIO) > 4000 && playing_eyes_close == false)
    {
        playing_eyes_close = true;
        current_anim_num_frames = 7;
        frame_counter = 0;
        current_anim_left = &ANIM_CLOSE_EYES;
        current_anim_right = &ANIM_CLOSE_EYES;

        delay(1000);
    }

#ifdef DEBUG_MUSIC
    if (music.available()) {
      printDetail(music.readType(), music.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }
#endif

    delay(250);


    if (playing_eyes_close)
    {
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
        delay(1000);
        esp_deep_sleep_start();
      }
    }


    // if phase is complete, move to next phase
    if (phase_complete[phase])
    {
        phase++;
        anim_idx += 2;

        if (phase == 3)
        {
            music.volume(10);
            Serial.println(music.readVolume());
            music.playFolder(1, 1);
        }

        // If that was the last phase, reset everything
        if (phase >= NUM_PHASES)
        {
            phase = 0;
            anim_idx = 0;

            for (int i = 0; i < NUM_PHASES; i++)
            {
                phase_complete[i] = 0;
            }

            playing_eyes_close = true;
            return;
        }

        is_new_phase = true;

#ifdef DEBUG
        Serial.println("Starting phase " + String(phase));
#endif
    }

    if (is_new_phase)
    {
        is_new_phase = false;

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
            Serial.println("Current time: " + String(current_time - current_anim_start_time) + " / " + String(current_anim_duration * 1000));
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


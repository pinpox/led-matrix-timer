#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ezButton.h>

// This optional setting causes Encoder to use more optimized code,
// It must be defined before Encoder.h is included.
// #define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>

#define ENCODER_CLK 3
#define ENCODER_DT  2
#define BUTTON_PIN  A4

ezButton button(BUTTON_PIN);  // create ezButton object that attach to pin A4;


// Font created with: https://pjrp.github.io/MDParolaFontEditor
// Add missing semicolon manually at the end after exporting.
#include "myfont.h"

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES  4
#define CLK_PIN     A1
#define DATA_PIN    A3
#define CS_PIN      A2

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

#define PAUSE_TIME  9000

unsigned long timeStart;
char textBuffer[80];
enum modes {
  MENU = 0,
  TIMER = 1,
  STOPWATCH = 2,
  ANIM_START = 3,
  ANIM_END = 4
};
modes opMode;
bool textBlink ;
long setTime;
long animStartTime;
void setup(void)
{
  Serial.begin(9600);
  Serial.println("Serial initialized");
  
  //Encoder
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, FALLING);


  //Display
  P.begin();
  P.setFont(newFont);

  // Button
  button.setDebounceTime(50); // set debounce time to 50 milliseconds

  //Software
  opMode = MENU;
  setTime = 0; //May be positive (for stopwatch) or negative (for countdown)
  animStartTime = 0;
}

void loop(void)
{
  button.loop(); // MUST call the loop() function first

  switch (opMode) {
    case STOPWATCH:
      {
        textBlink = false;
        int seconds = ((millis() - timeStart) / 1000) % 60;
        int minutes = ((millis() - timeStart) / 60000) % 60;
        sprintf(textBuffer, "_%02d:%02d", minutes, seconds );

        // Button was pressed (no animation)
        if (button.isPressed())
        {
          setTime = 0;
          opMode = MENU;
        }
        // Stopwatch expired
        if ( millis() - timeStart > setTime) {
          setTime = 0;
          animStartTime = millis();
          opMode = ANIM_END;
        }
      }
      break;
    case TIMER:
      {
        textBlink = false;
        long remaining = (millis() - timeStart + setTime);
        int seconds =  (remaining / -1000) % 60;
        int minutes =  (remaining / -60000) % 60;
        sprintf(textBuffer, "-%02d:%02d", minutes, seconds );

        // Button was pressed (no animation)
        if (button.isPressed() ) {
          setTime = 0;
          opMode = MENU;
        }
        // Timer expired
        if (remaining > 0) {
          setTime = 0;
          animStartTime = millis();
          opMode = ANIM_END;
        }
      }
      break;
    case MENU:
      {
        textBlink = true;
        int seconds = abs((setTime / 1000) % 60);
        int minutes = abs((setTime / 60000) % 60);

        //Show "-" prefix for negative times
        if (setTime < 0) {
          sprintf(textBuffer, "-%02d:%02d", minutes, seconds );
        } else {
          sprintf(textBuffer, "_%02d:%02d", minutes, seconds );
        }

        // Start stopwatch or timer
        if (button.isPressed()) {
          animStartTime = millis();
          opMode = ANIM_START;
        }
      }
      break;
    case ANIM_START:
      {
        textBlink = false;

        // Display animation
        sprintf(textBuffer, "%02d", 5 - (millis() - animStartTime) / 1000 );
        if (millis() - animStartTime > 5000) {
          sprintf(textBuffer, "__GO!" );
        }

        // Go to selected mode
        if (millis() - animStartTime > 6000) {
          timeStart = millis();
          if (setTime > 0) {
            opMode = STOPWATCH;
          } else {
            opMode = TIMER;
          }
        }
      }
      break;
    case ANIM_END:
      {

        // Display animation
        sprintf(textBuffer, "_GOOD" );
        if (millis() - animStartTime > 1000) {
          sprintf(textBuffer, "|_JOB!" );
        }
   if (millis() - animStartTime > 2000) {
     // SMILEY
          sprintf(textBuffer, "||||[" );
        }
        // Go to MENU mode
        if (millis() - animStartTime > 3000) {
          opMode = MENU;
        }
      }
      break;
    default:
      // Should never happen.
      {
        Serial.println("MODE: default");
      }
      break;
  }

  if (P.displayAnimate())
  {
    if ((millis() / 250) % 2 == 0 && textBlink) {
      P.setTextBuffer("");
    } else
    {
      P.setTextBuffer(textBuffer);
    }
    //Serial.println(String(textBuffer));
    P.displayReset();
  }
}


long lastEncoderChange = 0;

void readEncoder() {

  if (millis() - lastEncoderChange < 100) {
 
  return;
  }
  lastEncoderChange = millis();

  // Increase/decrease set time by 10s steps
  Serial.println("Reading encoder");

  int dtValue = digitalRead(ENCODER_DT);
  if (dtValue == HIGH) {
    Serial.println("Rotated clockwise");
    setTime += 1000 * 10;
  }
  if (dtValue == LOW) {
    Serial.println("Rotated counterclockwise");
    setTime -= 1000 * 10;
  }
  Serial.println(setTime);

}

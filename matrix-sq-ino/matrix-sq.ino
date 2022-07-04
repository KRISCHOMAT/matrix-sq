
////////**********\\\\\\\\
/////// Matrix Seq \\\\\\\\
 //////**  V 1.0  **\\\\\\
 // BY CHRISTIAN GROTHE \\
   // www.christian-grothe.com\\
////////**********\\\\\\\\
//**********************\\
#include "LedControl.h"
#include <Bounce2.h>
#include <EEPROM.h>

const uint8_t NUM_BUTTONS = 3;
const uint8_t NUM_BITOUT = 6;
const uint8_t NUM_BITS = 3;
const uint8_t DOTS = 36;
const uint8_t ROWS = 6;

const byte interruptPinA = 2;
const byte interruptPinB = 3;

volatile int counterA = 0;
volatile int counterB = 0;

bool Presets[6][DOTS] = {{}, {}, {}, {}, {}, {}};

#define din 13
#define clck 12
#define load 11

LedControl ledMatrix = LedControl(din, clck, load, 1);

int BitOut[NUM_BITOUT] = {8, 9, 10, 11, 12, 13};

// Matrix Buttons
Bounce *buttonsX = new Bounce[8];
Bounce *buttonsY = new Bounce[8];

// bit counts for 4051
int muxBit1;
int muxBit2;
int muxBit3;

// bit outs for 4051
int muxOut1 = 5;
int muxOut2 = 6;
int muxOut3 = 7;

int bits[ROWS];

int shownPreset = 0;
int usedPreset = 0;
int presetCount = 0;
bool setStates[6] = {0, 0, 0, 0, 0, 0};
bool lastStates[6] = {0, 0, 0, 0, 0, 0};
// const uint8_t trigger = 13;

int presetToggler = 0;

enum Menue
{
  main,
  setPresetCount,
  setChangePresets
};

Menue menue = main;

void setup()
{
  Serial.begin(9600);

  pinMode(interruptPinA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPinA), countingA, RISING);
  pinMode(interruptPinA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPinB), countingB, RISING);

  // Display Setup
  ledMatrix.shutdown(0, false);
  ledMatrix.setIntensity(0, 1);
  ledMatrix.clearDisplay(0);

  // reading from the EEPROM
  for (int i = 0; i < DOTS; i++)
  {

    Presets[0][i] = EEPROM.read(i);
    Presets[1][i] = EEPROM.read(i + DOTS);
    Presets[2][i] = EEPROM.read(i + DOTS * 2);
    Presets[3][i] = EEPROM.read(i + DOTS * 3);
    Presets[4][i] = EEPROM.read(i + DOTS * 4);
    Presets[5][i] = EEPROM.read(i + DOTS * 5);
  }

  // Set Up BitOut to 4051 SQ
  for (int i = 0; i < NUM_BITOUT; i++)
  {
    pinMode(BitOut[i], OUTPUT);
  }

  for (int i = 0; i < 8; i++)
  {
    buttonsX[i].attach(14, INPUT);
    buttonsX[i].interval(20);

    buttonsY[i].attach(15, INPUT);
    buttonsY[i].interval(20);
  }

  pinMode(muxOut1, OUTPUT);
  pinMode(muxOut2, OUTPUT);
  pinMode(muxOut3, OUTPUT);
}

void loop()
{
  // multiplexer for buttons
  for (int i = 0; i < 8; i++)
  {
    buttonsX[i].update();
    buttonsY[i].update();

    muxBit1 = bitRead(i, 0);
    muxBit2 = bitRead(i, 1);
    muxBit3 = bitRead(i, 2);

    digitalWrite(muxOut1, muxBit1);
    digitalWrite(muxOut2, muxBit2);
    digitalWrite(muxOut3, muxBit3);
  }

  // convert count to bits
  for (int i = 0; i < NUM_BITS; i++)
  {
    bits[i] = bitRead(counterA, i);
    bits[i + 3] = bitRead(counterB, i);
  }

  // change preset
  for (int i = 0; i < 6; i++)
  {
    if (buttonsY[6].read() == HIGH && buttonsX[i].rose())
    {
      shownPreset = i;
      usedPreset = i;
    }
  }

  // bit output to 4051 SQ
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(BitOut[i], sumofbits_function(i, presetCount));
  }

  // toggle menue

  if (buttonsY[7].rose())
  {
    menue = setPresetCount;
  }
  else if (buttonsY[6].rose())
  {
    menue = main;
  }
  else if (buttonsX[7].rose())
  {
    menue = setChangePresets;
    presetToggling();
  }
  else if (buttonsX[6].rose())
  {
    saveChangesToEEPROM();
  }

  switch (menue)
  {
  case main:
    showPresetOnMatrix(presetCount);
    changePreset(presetCount);
    presetCounter(bits);
    break;
  case setPresetCount:
    presetCounter(bits);
    configPresetCounter(bits);
    break;
  case setChangePresets:
    showPresetOnMatrix(presetToggler);
    changePreset(presetToggler);
    presetCounter(bits);
    break;

  default:
    break;
  }

} /// End Of Main Loop ///

// functions

void presetToggling()
{
  presetToggler++;

  if (presetToggler == 6)
  {
    presetToggler = 0;
  }
}

void configPresetCounter(int bits[])
{
  byte states[4] = {B00000000, B11100000, B00011100, B11111100};
  for (int i = 0; i < 6; i++)
  {
    if (buttonsX[i].rose())
    {
      setStates[i] = !setStates[i];
    }
    ledMatrix.setColumn(0, i, states[bits[i] + setStates[i] * 2]);
  }
}

void presetCounter(int bits[])
{
  for (int i = 0; i < 6; i++)
  {
    if (bits[i] != lastStates[i])
    {
      if (setStates[i])
      {
        presetCount++;
        if (presetCount == 6)
        {
          presetCount = 0;
        }
        lastStates[i] = bits[i];
      }
    }
  }
}

void changePreset(int preset)
{
  for (int i = 0; i < 6; i++)
  {

    if (buttonsX[i].rose() && buttonsY[0].read() == HIGH || buttonsX[i].read() == HIGH && buttonsY[0].rose())
    {
      Presets[preset][i] = !Presets[preset][i];
    }
    if (buttonsX[i].rose() && buttonsY[1].read() == HIGH || buttonsX[i].read() == HIGH && buttonsY[1].rose())
    {
      Presets[preset][i + 6] = !Presets[preset][i + 6];
    }
    if (buttonsX[i].rose() && buttonsY[2].read() == HIGH || buttonsX[i].read() == HIGH && buttonsY[2].rose())
    {
      Presets[preset][i + 6 * 2] = !Presets[preset][i + 6 * 2];
    }
    if (buttonsX[i].rose() && buttonsY[3].read() == HIGH || buttonsX[i].read() == HIGH && buttonsY[3].rose())
    {
      Presets[preset][i + 6 * 3] = !Presets[preset][i + 6 * 3];
    }
    if (buttonsX[i].rose() && buttonsY[4].read() == HIGH || buttonsX[i].read() == HIGH && buttonsY[4].rose())
    {
      Presets[preset][i + 6 * 4] = !Presets[preset][i + 6 * 4];
    }
    if (buttonsX[i].rose() && buttonsY[5].read() == HIGH || buttonsX[i].read() == HIGH && buttonsY[5].rose())
    {
      Presets[preset][i + 6 * 5] = !Presets[preset][i + 6 * 5];
    }
  }
}

void showPresetOnMatrix(int preset)
{
  for (int i = 0; i < ROWS; i++)
  {
    ledMatrix.setLed(0, 0, i, Presets[preset][i]);
    ledMatrix.setLed(0, 1, i, Presets[preset][i + ROWS]);
    ledMatrix.setLed(0, 2, i, Presets[preset][i + ROWS * 2]);
    ledMatrix.setLed(0, 3, i, Presets[preset][i + ROWS * 3]);
    ledMatrix.setLed(0, 4, i, Presets[preset][i + ROWS * 4]);
    ledMatrix.setLed(0, 5, i, Presets[preset][i + ROWS * 5]);
  }
}

bool sumofbits_function(int column, int preset)
{
  bool sumofbits = 0;

  if (bits[0] && Presets[preset][column])
  {
    sumofbits = 1;
  }
  else if (bits[1] && Presets[preset][column + 6])
  {
    sumofbits = 1;
  }
  else if (bits[2] && Presets[preset][column + 12])
  {
    sumofbits = 1;
  }
  else if (bits[3] && Presets[preset][column + 18])
  {
    sumofbits = 1;
  }
  else if (bits[4] && Presets[preset][column + 24])
  {
    sumofbits = 1;
  }
  else if (bits[5] && Presets[preset][column + 30])
  {
    sumofbits = 1;
  }
  else
  {
    sumofbits = 0;
  }

  return sumofbits;
}

void countingA()
{
  counterA++;
  if (counterA == 8)
  {
    counterA = 0;
  }
}

void countingB()
{
  counterB++;
  if (counterB == 8)
  {
    counterB = 0;
  }
}

void saveChangesToEEPROM()
{
  for (int i = 0; i < DOTS; i++)
  {
    EEPROM.update(i, Presets[0][i]);
    EEPROM.update(i + DOTS, Presets[1][i]);
    EEPROM.update(i + (DOTS * 2), Presets[2][i]);
    EEPROM.update(i + (DOTS * 3), Presets[3][i]);
    EEPROM.update(i + (DOTS * 4), Presets[4][i]);
    EEPROM.update(i + (DOTS * 5), Presets[5][i]);
  }
}
// Include the Arduino Stepper Library
#include <Stepper.h>
#include <EEPROM.h>

#include "Monochromator.h"

#define VALUE_CURRENT_WAVELENGTH 0
#define VALUE_MIN_WAVELENGTH 1
#define VALUE_MAX_WAVELENGTH 2
#define VALUE_STEP_NM     3

// Number of steps per output rotation
#define stepsPerRevolution 200

Stepper stepper(stepsPerRevolution, 7, 8, 9, 10);

void MonochromatorInit()
{
    stepper.setSpeed(110);
}

void MonochromatorConfigure(uint32_t CurrentWavelength, uint32_t MinWavelength, uint32_t MaxWavelength, uint16_t StepsPerNm, uint8_t StepDenom)
{
    SaveValue(VALUE_CURRENT_WAVELENGTH, CurrentWavelength);
    SaveValue(VALUE_MIN_WAVELENGTH, MinWavelength);
    SaveValue(VALUE_MAX_WAVELENGTH, MaxWavelength);
    SaveValue(VALUE_STEP_NM, StepsPerNm);
}

static void set_wavelength(uint32_t Wavelength)
{
    int32_t diff = Wavelength - GetValue(VALUE_CURRENT_WAVELENGTH);
    int32_t steps = diff * GetValue(VALUE_STEP_NM);
    
    /* Move*/
    stepper.step(steps);

    /* Backlash: always finish with a positive rotation */
    if (steps < 0)
    {
        stepper.step(-35);
        stepper.step(+35);
    }

    /* Motor is now draining all the arduino's power, so set all motor pins to zero. */
//    digitalWrite(7, LOW);
//    digitalWrite(8, LOW);
//    digitalWrite(9, LOW);
//    digitalWrite(10, LOW);
}

int MonochromatorSetWavelength(uint32_t Wavelength)
{
    set_wavelength(Wavelength);
    SaveValue(VALUE_CURRENT_WAVELENGTH, Wavelength);
    return 0;
}

uint32_t MonochromatorGetWavelength()
{
    return GetValue(VALUE_CURRENT_WAVELENGTH);
}

static union {
  uint32_t int32;
  uint8_t bytes[4];
} value;

static void SaveValue(int ID, uint32_t Value)
{
    value.int32 = Value;
    for (int i = 0; i < 4; ++i) EEPROM.write(ID*4+i, value.bytes[i]);
}

static uint32_t GetValue(int ID)
{
    for (int i = 0; i < 4; ++i) value.bytes[i] = EEPROM.read(ID*4+i);
    return value.int32;
}

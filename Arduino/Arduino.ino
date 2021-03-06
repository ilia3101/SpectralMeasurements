#include <Servo.h>

/*  IMPORTANT:
    When manually presetting the wavelength (632.8nm in this case), always go up in value,
    so for example, set it to 620 first, then turn it up to 632.8, for consistent backlash
    (always finish on a positive rotation)
*/

// How long to let the camera write to card in milliseconds. Increase if an old camera is unable to keep up
#define CAMERA_BUFFERING_TIME 4000
/* Set camera to 6 seconds, so diode integrates within shutter beginning and ending */
#define DIODE_INTEGRATION_TIME 3750 /* Because TSL235 is terrible we must have such a long integration time */

/*********************************************
    OLED display
 *********************************************/

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

Adafruit_SSD1306 display(-1);

/*********************************************
    Shutter presser
 *********************************************/

Servo shutter_presser;

void PressShutter()
{
    int angle = 0;
    int max_angle = 40;
    shutter_presser.write(max_angle);
    delay(250);
    shutter_presser.write(0);
    delay(50); // CHANGE THIS BAC TO 70 and the first delay to 220 or smthng
}


/*********************************************
    WAVELENGTH INFO
    In 10 * nanometres
 *********************************************/

#define WAVELENGTH_START_POS 6328 /* HeNe Laser calibration wavelength. Set to this wavelength manually before running everything! */
#define WAVELENGTH_START 3800 /* Will start taking measurements from this wavelength (380nm) */
#define WAVELENGTH_END 7500 /* Will keep measuring until this wavelength is reached (730nm) */
#define WAVELENGTH_STEP 20 /* Wavelength step, 2nm and 5nm are best */

#define STEPS_PER_REVOLUTION ((int32_t)200) /* For stepper */
#define WAVELENGTH_PER_REVOLUTION 250 /* 25nm is one full rotation */


/*********************************************
    Filter wheel.
    To hopefully improve monochromator output
 *********************************************/

Servo filterwheel;
#define FilterWheelPos_CLOSED 34 /* An opaque filter consisting of foil, for getting dark frame and dark current before each measurement */
#define FilterWheelPos_NOFILTER 180 /* No filter, for wavelengths between 420 and 645nm */
#define FilterWheelPos_VIOLET_420 69 /* bandpass 380-420 (very sharp cutoffs) */
#define FilterWheelPos_RED_645 0 /* SChott RG645 for measuring wavelengths 645nm and over */

void SetFilterWheel(int FilterOption)
{
    filterwheel.write(FilterOption);
    /* Let the servo settle... */
    delay(400);
}

/* Sets filter automatically for wavelength */
void SetFilterForWavelength(uint32_t wavelength)
{
    if (wavelength >= 3800 && wavelength < 4200) {
        SetFilterWheel(FilterWheelPos_VIOLET_420);
    } else if (wavelength >= 6480) {
        SetFilterWheel(FilterWheelPos_RED_645);
    } else {
        SetFilterWheel(FilterWheelPos_NOFILTER);
    }
}


/*********************************************
    Photodiode (TSL235)
 *********************************************/

volatile uint32_t cnt = 0;
void interrupr() {
    ++cnt;
}

void DiodeInit()
{
    pinMode(2, INPUT);
    digitalWrite(2, HIGH);
}

uint32_t DiodeMeasure(uint32_t IntegrationTime)
{
    cnt = 0;
    attachInterrupt(0, interrupr, CHANGE);
    delay(IntegrationTime);
    detachInterrupt(0);
    return cnt;
}


/*********************************************
    Stepper motor (A4988 controller configured for microstepping)
 *********************************************/
int stepPin = 12;
int dirPin  = 13;
int  enPin  = 10;

int32_t steps = 0;

void StepperStep(int32_t numsteps)
{
    /*  All values between 60 and 250 seem ok, I chose this lower
        speed because it seems to vibrate less than others, there's
        probably a better value, I didn't spend long choosing */
    const int steptime = 220;
    
    steps += numsteps;

    // Set the spinning direction
    if (numsteps < 0) {
        digitalWrite(dirPin, LOW);
        numsteps = -numsteps;
    } else {
        digitalWrite(dirPin, HIGH);
    }

    delay(50);

    // Spin the stepper motor
    for (int32_t s = 0; s < numsteps * 16; s++)
    {
        // These four lines result in 1 step:
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(steptime);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(steptime);
    }

    delay(5);
}

void StepperInit()
{
     pinMode(stepPin,OUTPUT);

     pinMode(dirPin,OUTPUT);

     pinMode(enPin , OUTPUT);

     digitalWrite(stepPin, LOW);
     digitalWrite(dirPin, HIGH);
     digitalWrite( enPin , HIGH);
}

void SetWavelength(int32_t TargetWavelength)
{
    double current_wavelength = WAVELENGTH_START_POS + ((double)steps * (double)WAVELENGTH_PER_REVOLUTION) / (double)STEPS_PER_REVOLUTION;
    double wl_diff = (double)TargetWavelength - current_wavelength;
    double steps_required = ((double)wl_diff * (double)STEPS_PER_REVOLUTION) / (double)WAVELENGTH_PER_REVOLUTION;
    int32_t steps_required_int = steps_required;
    if (steps_required < 0) steps_required_int = (int32_t)(steps_required - 0.5);
    else steps_required_int = (int32_t)(steps_required + 0.5);

    /* Backlash correction - always finish on a positive rotation */
    int32_t backlash_steps = 50;
    if (steps_required < 0)
    {
        StepperStep(steps_required - backlash_steps);
        delay(20);
        StepperStep(+backlash_steps);
    }
    else
    {
        StepperStep(steps_required);
    }
}


/*********************************************
 * General functions
 *********************************************/

// Reads diode, outputs to serial and presses the shutter
void TakeReading()
{
    /* Camera reading */
    PressShutter();

    /* Diode reading */
    uint32_t diode_value = DiodeMeasure(DIODE_INTEGRATION_TIME);
    Serial.println(diode_value);

    /* Allow camera to write or whatever */
    delay(CAMERA_BUFFERING_TIME);
}


void setup()
{
    Serial.begin(9600);

    /****************** TSL235 diode ******************/
    DiodeInit();

    /****************** FILTER WHEEL ******************/
    filterwheel.attach(5);
    SetFilterWheel(FilterWheelPos_CLOSED); /* Keep mostly closed cause the lamp makes a lot of heat */

    //SetFilterWheel(FilterWheelPos_RED_645);
    //SetFilterWheel(FilterWheelPos_VIOLET_420);
    //while(1);
    /****************** SHUTTER PRESSER ******************/
    shutter_presser.attach(4);
    shutter_presser.write(0);

    /****************** STEPPER ******************/
    StepperInit();
    uint32_t original_pos = WAVELENGTH_START_POS;

    /****************** DISPLAY ******************/
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(2,2);
    display.print("Plug in stepper motor NOW");
    display.display();

    /* wait (time to plug in stepper) */
    delay(8000);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(2,2);
    display.print("Setting start wavelength");
    display.display();

    /*********************************************
        Take actual measurements now
     *********************************************/
    if (WAVELENGTH_START-WAVELENGTH_STEP > 3000)
      SetWavelength(WAVELENGTH_START-WAVELENGTH_STEP);
    else
      SetWavelength(300);

    uint32_t time_start = millis();

    for (int32_t wl = WAVELENGTH_START; wl <= WAVELENGTH_END; wl += WAVELENGTH_STEP)
    {
        /* Set wavelength */
        SetWavelength(wl);

        /* Dark reading */
        TakeReading();

        /* Main reading */
        SetFilterForWavelength(wl);
        TakeReading();
        SetFilterWheel(FilterWheelPos_CLOSED);

        uint32_t time_now = millis();
        uint32_t time_per_reading = (time_now - time_start) / ((wl-WAVELENGTH_START)/WAVELENGTH_STEP + 1);
        uint32_t time_left = time_per_reading * ((WAVELENGTH_END-wl)/WAVELENGTH_STEP);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(2,2);
        display.print("Minutes left: ");
        display.print(((double)time_left)/60000.0);
        display.setCursor(2,12);
        display.print("Wavelength: ");
        display.print(((double)wl)/10.0);
        display.display();
    }

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(2,2);
    display.print("Finished!");
    display.display();

    SetFilterWheel(FilterWheelPos_CLOSED);
    SetWavelength(original_pos);

    while (1);
}

void loop()
{
}

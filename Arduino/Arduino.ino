#include <Servo.h>

/*  IMPORTANT:
    When manually presetting the wavelength (633nm in this case), always go up in value,
    so for example, set it to 620 first, then turn it up to 633, for consistent backlash
    (always finish on a positive rotation)
*/

// How long to let the camera write to card in milliseconds. Increase if an old camera unable to keep up
#define CAMERA_BUFFERING_TIME 3000 /* Time for shooting *and* recording to card. Leave enough! */
#define DIODE_INTEGRATION_TIME 2000 /* Because the diode is terrible we must have such a long integration time */


/*********************************************
    Shutter presser
 *********************************************/

Servo shutter_presser;

void PressShutter()
{
    int angle = 0;
    int max_angle = 22;
    shutter_presser.write(max_angle);
    delay(50);
    shutter_presser.write(0);
    delay(20);
}


/*********************************************
    WAVELENGTH INFO
    In 10 * nanometres
 *********************************************/

#define WAVELENGTH_START_POS 6332 /* HeNe Laser calibration wavelength. Set to this wavelength manually before running everything! */
#define WAVELENGTH_START 3800 /* Will start taking measurements from this wavelength (380nm) */
#define WAVELENGTH_END 7200 /* Will keep measuring until this wavelength is reached (720nm) */
#define WAVELENGTH_STEP 20 /* Wavelength step, 2nm and 5nm are best */

#define STEPS_PER_REVOLUTION ((int32_t)200) /* For stepper */
#define WAVELENGTH_PER_REVOLUTION 250 /* 25nm is one full rotation */

double current_wavelength = WAVELENGTH_START_POS;


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
    if (wavelength >= 3800 && wavelength <= 4200) {
        SetFilterWheel(FilterWheelPos_VIOLET_420);
    } else if (wavelength > 6450) {
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

#define dirPin 13
#define stepPin 12
#define powPin 11

void StepperStep(int32_t numsteps)
{
    /*  All values between 60 and 250 seem ok, I chose this lower
        speed because it seems to vibrate less than others, there's
        probably a better value, I didn't spend long choosing */
    const int steptime = 220;

    // Set the spinning direction
    if (numsteps < 0) {
        digitalWrite(dirPin, HIGH);
        numsteps = -numsteps;
    } else {
        digitalWrite(dirPin, LOW);
    }

    // Spin the stepper motor
    //Step loss is always in multiples of 4 according to somewhere on the internet, differnt value sseem to work best on different days :/
    int32_t step_loss = 4*0;
    for (int32_t s = 0; s < numsteps * 16 + step_loss; s++)
    {
        // These four lines result in 1 step:
        delayMicroseconds(steptime);
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(steptime);
        digitalWrite(stepPin, LOW);
    }
}

void StepperInit()
{
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(powPin, OUTPUT);

    /* Holding torque should exist after this */
    digitalWrite(powPin, HIGH);

    /* Delay always helps thigns settle */
    delay(100);
}

void SetWavelength(int32_t TargetWavelength)
{
    double wl_diff = (double)TargetWavelength - current_wavelength;
    double steps_required = ((double)wl_diff * (double)STEPS_PER_REVOLUTION) / (double)WAVELENGTH_PER_REVOLUTION;
    int32_t steps_required_int;
    if (steps_required < 0) steps_required_int = (int32_t)(steps_required - 0.5);
    else steps_required_int = (int32_t)(steps_required + 0.5);

    String str;
    //Serial.println(str + "Steps required: " + steps_required_int);

    /* Backlash correction - always finish on a positive rotation */
    int32_t backlash_steps = 30;
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

    current_wavelength = current_wavelength + ((double)steps_required_int / (double)STEPS_PER_REVOLUTION) * (double)WAVELENGTH_PER_REVOLUTION;

    //Serial.println(str + "New wavelength: " + current_wavelength/10 + "nm");
}


/*********************************************
 * General functions
 *********************************************/

// Reads diode, outputs to serial and presses the shutter
void TakeReading()
{
    /* Diode reading */
    uint32_t diode_value = DiodeMeasure(DIODE_INTEGRATION_TIME);
    Serial.println(diode_value);

    /* Camera reading */
    PressShutter();

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
    SetFilterWheel(FilterWheelPos_CLOSED);

    /****************** SHUTTER PRESSER ******************/
    shutter_presser.attach(4);
    PressShutter();

    /****************** STEPPER ******************/
    StepperInit();
    uint32_t prev = current_wavelength;

    /*********************************************
        Take actual measurements now
     *********************************************/

    for (int32_t wl = WAVELENGTH_START; wl <= WAVELENGTH_END; wl += WAVELENGTH_STEP)
    {
        /* Set wavelength */
        SetWavelength(wl);

        /* Dark reading */
        SetFilterWheel(FilterWheelPos_CLOSED);
        TakeReading();

        /* Main reading */
        SetFilterForWavelength(wl);
        TakeReading();
    }

    SetFilterWheel(FilterWheelPos_CLOSED);
    SetWavelength(prev);

    while (1);
}
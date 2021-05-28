#include "MessageSender.h"

#include "Control.h"

static int arduino_fd;
static ArduinoMsg_t msg;
static ArduinoResponse_t response;

int InitMeasurementSystem()
{
    arduino_fd = OpenArduino();

    if (arduino_fd < 0) return 1;
    else return 0;
}

void FinishMeasurement()
{
    CloseArduino(arduino_fd);
}

static uint32_t send()
{
    ArduinoSendMsg(arduino_fd, &msg);
    ArduinoRecieveResponse(arduino_fd, &response);
    return response.value;
}

uint32_t SetMonochromatorWavelength(int Wavelength)
{
    msg.type = Arduino_SET_WAVELENGHTH;
    msg.value_a = Wavelength;

    return send();
}

uint32_t MonochromatorCalibrate(int CurrentWavelength, int MaxWavelength, int MinWavelength, int StepsPerNm)
{
    msg.type = Arduino_INITIALISE_MONOCHROMATOR;
    msg.value_a = CurrentWavelength;
    msg.value_b = MaxWavelength;
    msg.value_c = MinWavelength;
    msg.value_d = StepsPerNm;
    msg.value_e = 1;

    return send();
}

uint32_t SetIngtegrationTime(uint32_t IntegrationTime)
{
    msg.type = Arduino_SET_INTEGRATION_TIME;
    msg.value_a = IntegrationTime;

    return send();
}

uint32_t TakeReading()
{
    msg.type = Arduino_TAKE_READING;
    msg.value_a = 0;
    return send();
}

uint32_t TakeReadingOnlyDiode()
{
    msg.type = Arduino_TAKE_READING;
    msg.value_a = 1;
    return send();
}
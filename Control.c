#include "MessageSender.h"

#include "Control.h"

static int arduino_fd;
static ArduinoMsg_t msg;
static ArduinoResponse_t response;

int InitMeasurementSystem(char * DeviceFile)
{
    arduino_fd = OpenArduino(DeviceFile);

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

int SetMonochromatorWavelength(int Wavelength)
{
    msg.type = Arduino_SET_WAVELENGHTH;
    msg.value_a = Wavelength;

    return send();
}

int MonochromatorCalibrate(int CurrentWavelength, int MaxWavelength, int MinWavelength, int StepsPerNm)
{
    msg.type = Arduino_INITIALISE_MONOCHROMATOR;
    msg.value_a = CurrentWavelength;
    msg.value_b = MaxWavelength;
    msg.value_c = MinWavelength;
    msg.value_d = StepsPerNm;
    msg.value_e = 1;

    return send();
}

int SetIngtegrationTime(uint32_t IntegrationTime)
{
    msg.type = Arduino_SET_INTEGRATION_TIME;
    msg.value_a = IntegrationTime;

    return send();
}

uint32_t TakeReading()
{
    msg.type = Arduino_TAKE_READING;
    return send();
}
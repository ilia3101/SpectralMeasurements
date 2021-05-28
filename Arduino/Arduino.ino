#include "Diode.h"
#include "Servo.h"
#include "Monochromator.h"

#include "Message.h"

int exposure_ms = 2000;

void setup()
{
    Serial.begin(9600);
    ServoInit();
    DiodeInit();
    MonochromatorInit();
}

ArduinoMsg_t message;
ArduinoResponse_t response;

void loop ()
{
    if (Serial.available() == sizeof(ArduinoMsg_t))
    {
        SerialReceiveMessage(&message);
        
        switch (message.type)
        {
            case Arduino_TAKE_READING:
            {
                ServoPressShutter();
                response.value = DiodeMeasure(exposure_ms);  
                break;
            }
            case Arduino_SET_INTEGRATION_TIME:
            {
                exposure_ms = message.value_a;
                response.value = 0;
                break;
            }
            case Arduino_SET_WAVELENGHTH:
            {
                response.value = MonochromatorSetWavelength(message.value_a);
                break;
            }
            case Arduino_INITIALISE_MONOCHROMATOR:
            {
                /* Informm about monochromator */
                MonochromatorConfigure(message.value_a, message.value_b, message.value_c, message.value_d, message.value_e);
                response.value = 0;
                break;
            }
            default:
            {
                /* Something is very wrong, stop forever */
                while (true);
            }
        }

        SerialSendResponse(&response);
    }
}

void SerialReceiveMessage(ArduinoMsg_t * Msg)
{
    uint8_t * msgdata = (uint8_t *)(void *)Msg;
    for (int i = 0; i < sizeof(ArduinoMsg_t); ++i)
    {
        msgdata[i] = Serial.read();
    }
}

void SerialSendResponse(ArduinoResponse_t * Response)
{
    int n = Serial.write((uint8_t *)Response, sizeof(ArduinoResponse_t));
}


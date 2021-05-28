#include "Diode.h"
#include "Servo.h"
#include "Monochromator.h"

#include "Message.h"

// the default value, will get overriden by SET_INTEGRATION_TIME message
int exposure_ms = 2000;

// number of bytes received
int num_bytes = 0;
uint8_t * buf = 0;
ArduinoMsg_t message;
ArduinoResponse_t response;

void setup()
{
    Serial.begin(9600);
    ServoInit();
    DiodeInit();
    MonochromatorInit();
    buf = (uint8_t *)&message;
    // clear all serial to avoid freezing 
    //while (Serial.available()) Serial.read();
}

void loop ()
{
    // read a serial bytes if available
    if (Serial.available())
    {
        buf[num_bytes] = Serial.read();
        ++num_bytes;
    }
    if (num_bytes == sizeof(ArduinoMsg_t))
    {
        num_bytes = 0;
        
        switch (message.type)
        {
            case Arduino_TAKE_READING:
            {
                if (!message.value_a) ServoPressShutter();
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
            case Arduino_TEST:
            {
                response.value = message.value_a * message.value_b;
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


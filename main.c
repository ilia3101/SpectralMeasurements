//rm a.out; gcc -c MessageSender.c; gcc -c main.c; gcc *.o; rm *.o; ./a.out

#include <stdio.h>

#include "MessageSender.h"

int main(int argc, char *argv[])
{
    int fd = OpenArduino("/dev/ttyUSB1");

    ArduinoMsg_t msg;
    ArduinoResponse_t response;

    int counter = 0;

    // msg.type = Arduino_INITIALISE_MONOCHROMATOR;
    // msg.value_a = 849; /* Current position */
    // msg.value_b = 320; /* Min wavelength */
    // msg.value_c = 850; /* Max wavelfnth */
    // msg.value_d = 8; /* Motor steps per nanometer */
    // msg.value_e = 1; /* Denominator for previous value */
    // ArduinoSendMsg(fd, &msg);
    // ArduinoRecieveResponse(fd, &response);


    msg.type = Arduino_SET_WAVELENGHTH;
    msg.value_a = 840;
    ArduinoSendMsg(fd, &msg);
    ArduinoRecieveResponse(fd, &response);

    while (1)
    {
        // msg.type = Arduino_SET_INTEGRATION_TIME;
        // msg.value_a = (1+counter) * 500;
        // ArduinoSendMsg(fd, &msg);
        // ArduinoRecieveResponse(fd, &response);

        msg.type = Arduino_TAKE_READING;
        msg.value_a = 1000;
        ArduinoSendMsg(fd, &msg);
        ArduinoRecieveResponse(fd, &response);

        /* print what's in the buffer */
        printf("Buffer contains...\n%i\n", response.value);

        counter = (counter + 1) % 4;
    }

    return 0;
}
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "MessageSender.h"

/* Return value is descriptor. Negative return value means error */
int OpenArduino(char * PortName)
{
    int fd;

    /* Open the file descriptor in non-blocking mode */
    fd = open(PortName, O_RDWR | O_NOCTTY);

    if (fd < 0) return fd;

    /* Set up the control structure */
    struct termios toptions;

    /* Get currently set options for the tty */
    tcgetattr(fd, &toptions);

    /* Set options */
    /* set 9600 baud both ways */
    cfsetispeed(&toptions, B9600);
    cfsetospeed(&toptions, B9600);
    /* 8 bits, no parity, no stop bits */
    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;
    /* Canonical mode */
    // toptions.c_lflag |= ICANON;
    /* wait for 12 characters to come in before read returns */
    /* WARNING! THIS CAUSES THE read() TO BLOCK UNTIL ALL */
    /* CHARACTERS HAVE COME IN! */
    toptions.c_cc[VMIN] = sizeof(ArduinoResponse_t);
    /* commit the serial port settings */
    tcsetattr(fd, TCSANOW, &toptions);

    /* Wait for the Arduino to reset */
    sleep(2);

    return fd;
}

void CloseArduino(int Descriptor)
{
    close(Descriptor);
}

/* Send a message */
int ArduinoSendMsg(int Descriptor, ArduinoMsg_t * Message)
{
    int n = write(Descriptor, Message, sizeof(ArduinoMsg_t));
    if (n != sizeof(ArduinoMsg_t)) return 1;
    else return 0;
}

/* Wait for and programreceive message, return value is number of bytes. */
int ArduinoRecieveResponse(int Descriptor, ArduinoResponse_t * Output)
{
    // fd_set set;
    // struct timeval timeout;

    // /* Initialize the file descriptor set. */
    // FD_ZERO (&set);
    // FD_SET (Descriptor, &set);

    // /* Initialize the timeout data structure. */
    // timeout.tv_sec = 100;
    // timeout.tv_usec = 0;

    // /* select returns 0 if timeout, 1 if input available, -1 if error. */
    // switch (select(FD_SETSIZE, &set, NULL, NULL, &timeout))
    // {
    //     case 1:
            usleep(1000 * 10); // sleep for 10 milliseconds
            int n = read(Descriptor, Output, sizeof(ArduinoMsg_t));
            if (n != sizeof(ArduinoResponse_t)) return 1;
            else return 0;
    //     default:
    //         return 1;
    // }
}
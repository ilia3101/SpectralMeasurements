#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include "MessageSender.h"

/* Return value is descriptor. Negative return value means error */
int _OpenArduino(char * PortName)
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
    /* enable receiver, ignore status lines */
    toptions.c_cflag |= CREAD | CLOCAL;
    /* disable input/output flow control, disable restart chars */
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY);
    /* disable canonical input, disable echo,
    disable visually erase chars,
    disable terminal-generated signals */
    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    /* disable output processing */
    toptions.c_oflag &= ~OPOST;
    /* commit the serial port settings */
    tcsetattr(fd, TCSANOW, &toptions);

    /* Wait for the Arduino to reset */
    sleep(2);

    int success = 0;
    ArduinoMsg_t test = {
        .type = Arduino_TEST,
        .value_a = (rand()%100) + 1,
        .value_b = (rand()%100) + 1
    };
    ArduinoResponse_t response;
    if (ArduinoSendMsg(fd, &test)) return -1;
    ArduinoRecieveResponse(fd, &response);

    if (response.value = test.value_a * test.value_b)
        return fd;
    else
        return -1;
}

int OpenArduino()
{
    srand(time(NULL));
    for (int i = 0; i < 100; ++i)
    {
        char port_name[100];
        sprintf(port_name, "/dev/ttyUSB%i", i);

        int fd = _OpenArduino(port_name);
        if (fd > 0) return fd;
    }

    puts("Did not succesfully find connected Arduino, exiting");
    exit(1);
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
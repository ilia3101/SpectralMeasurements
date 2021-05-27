#ifndef _Arduino_h_
#define _Arduino_h_

#include "Arduino/Message.h"

/* Arduino communication, 100% synchronous only !!! */

/* Return value is descriptor. Negative return value means error */
int OpenArduino();
void CloseArduino(int Descriptor);

/* Send a message. Return 0 = Ok */
int ArduinoSendMsg(int Descriptor, ArduinoMsg_t * Message);

/* Wait for and receive message. Return 0 = Ok */
int ArduinoRecieveResponse(int Descriptor, ArduinoResponse_t * Response);

#endif
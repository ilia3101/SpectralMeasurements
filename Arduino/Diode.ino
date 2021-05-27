#include "Diode.h"

volatile uint32_t cnt = 0;
void interrupr() { ++cnt; }

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

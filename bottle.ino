#include <HCSR04.h>

HCSR04 hc(1, 2); //initialisation class HCSR04 (trig pin , echo pin)

void setup()
{
    Serial.begin(9600);
}

void loop()
{
    Serial.println(hc.dist()); 
    delay(60);                 
}

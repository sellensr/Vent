#include <Servo.h>

Servo servoCPAP, servoPEEP, servoDual;  // create servo object to control a servo
void setup()
{
  Serial.begin(115200);

  Serial.print("\n\nRWS Vent_Go901\n\n");
  servoDual.attach(11);  
  servoCPAP.attach(10);  
  servoPEEP.attach(9);
  servoDual.write(90);
  servoCPAP.write(90);  
  servoPEEP.write(90); 

  delay(100); 
}

 
void loop()
{
  servoDual.write(90);
  servoCPAP.write(90);
  servoPEEP.write(90);
}

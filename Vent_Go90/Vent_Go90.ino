#include <Servo.h>
#define ALARM_PIN 5

Servo servoCPAP, servoPEEP, servoDual;  // create servo object to control a servo
void setup()
{
  Serial.begin(115200);

  Serial.print("\n\nRWS Vent_Go901\n\n");
  servoDual.attach(11);  
  servoCPAP.attach(10);  
  servoPEEP.attach(9);
  servoDual.write(80);
  servoCPAP.write(80);  
  servoPEEP.write(80); 
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, HIGH);
  delay(5); ///< just long enough to make a little squeak
  digitalWrite(ALARM_PIN, LOW);

  delay(1000); 
}

 
void loop()
{
  servoDual.write(90);
  servoCPAP.write(90);
  servoPEEP.write(90);
}


void ultrasonic_up()
{ 
  digitalWrite(trig_up, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_up, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_up, LOW);
  up_duration = pulseIn(echo_up, HIGH);
  up_distance= up_duration*0.034/2;
  
  Serial.print("Up_Distance: ");
  Serial.println(up_distance);
  }

// Chương trình con
void forward(){ // chương trình con xe robot đi tiến
  digitalWrite(motorA1,LOW);
  digitalWrite(motorA2,HIGH);                       
  digitalWrite(motorB2,HIGH);
  digitalWrite(motorB1,LOW);
}

void back(){ // chương trình con xe robot đi tiến
  digitalWrite(motorA2,LOW);
  digitalWrite(motorA1,HIGH);                       
  digitalWrite(motorB1,HIGH);
  digitalWrite(motorB2,LOW);
}

void turnRight(){
  digitalWrite(motorA1,HIGH);
  digitalWrite(motorA2,LOW);                       
  digitalWrite(motorB2,HIGH);
  digitalWrite(motorB1,LOW);
}

void turnLeft(){
  digitalWrite(motorA1,LOW);
  digitalWrite(motorA2,HIGH);                       
  digitalWrite(motorB1,HIGH);
  digitalWrite(motorB2,LOW);
}

void Stop(){
  digitalWrite(motorA1,LOW);
  digitalWrite(motorA2,LOW);                       
  digitalWrite(motorB1,LOW);
  digitalWrite(motorB2,LOW);
}

void turn_180(){
  digitalWrite(motorA1,LOW);
  digitalWrite(motorA2,LOW);                       
  digitalWrite(motorB1,LOW);
  digitalWrite(motorB2,LOW);
}

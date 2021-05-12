void sensor_ir(){
  left_sensor_state = digitalRead(L_S);
  right_sensor_state = digitalRead(R_S);
  ball_detect_state = digitalRead(ball_detect);
  
  if ((digitalRead(L_S) == 0)&&(digitalRead(R_S) == 0)){forward();Serial.println("forward");}// đi tiến 
  if ((digitalRead(L_S) == 1)&&(digitalRead(R_S) == 0)){turnLeft();Serial.println("turnLeft");} // rẻ trái
  if ((digitalRead(L_S) == 0)&&(digitalRead(R_S) == 1)){turnRight();Serial.println("turnRight");} // rẻ phải
  if ((digitalRead(L_S) == 1)&&(digitalRead(R_S) == 1)){turn_180();Serial.println("turn_180");} // quay xe
}

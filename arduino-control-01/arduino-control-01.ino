/*
mô tả:
code Arduino UNO R3: cho xe chạy tự do tránh vật cản và khu vực thi đấu
Baund Rate : 9600




*/

int stop_distance = 12; // Khoảng cách phát hiện vật cản 12cm

//Kết nối sensor ultrasonic HC-SR 04 trên
const int trigPin_up = 11; // kết nối chân trig với chân 11 arduino
const int echoPin_up = 12; // kết nối chân echo với chân 12 arduino

//Kết nối sensor ultrasonic HC-SR 04 dưới
const int trigPin_down = 9; // kết nối chân trig với chân 11 arduino
const int echoPin_down = 10; // kết nối chân echo với chân 12 arduino

//Khai báo chân interrupt
int interrupt_pin = 2;


//L298 kết nối arduino
const int motorA1      = 3; // kết nối chân IN1 với chân 3 arduino
const int motorA2      = 4; // kết nối chân IN2 với chân 4 arduino
const int motorAspeed  = 5; // kết nối chân ENA với chân 5 arduino

const int motorB1      = 7; // kết nối chân IN3 với chân 7 arduino
const int motorB2      = 8; // kết nối chân IN4 với chân 8 arduino
const int motorBspeed  = 6; // kết nối chân ENB với chân 6 arduino

//kết nối của 3 cảm biến hồng ngoại (dò line )
const int IR_right = A0;  // cb dò line trái
const int IR_left = A1;  // cb dò line giữa
const int IR_detect_object = A2;  // cb dò line giữa

int left_sensor_state = 0; // biến lưu cảm biến hồng ngoại line trái
int right_sensor_state = 0; // biến lưu cảm biến hồng ngoại line phải
int object_sensor_state = 0; // biến lưu cảm biến hồng ngoại phat hien vat the

long duration; // 
int distance;  // biến khoảng cách

void setup() {
  pinMode(IR_right, INPUT); // chân cảm biến khai báo là đầu vào
  pinMode(IR_left, INPUT);
  pinMode(IR_detect_object, INPUT); 
  
  pinMode(motorA1, OUTPUT); 
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(motorAspeed, OUTPUT);
  pinMode(motorBspeed, OUTPUT);
  
  pinMode(trigPin, OUTPUT); // sensor HC-SR04
  pinMode(echoPin, INPUT); 
    
  Serial.begin(9600); 
  analogWrite(motorAspeed, 120); // tốc độ động cơ a ban đầu 120 ( 0 - 255)
  analogWrite(motorBspeed, 120);// tốc độ động cơ b ban đầu 120 ( 0 - 255)
  delay(3000);                              
  
}

void loop() 
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance= duration*0.034/2;
  
  Serial.print("Distance: ");
  Serial.println(distance);

  left_sensor_state = digitalRead(IR_right);
  right_sensor_state = digitalRead(IR_left);
  object_sensor_state = digitalRead(IR_detect_object);

  
  /*
  khi xe phát hiện đường lane màu trắng trong phần sân thi đấu sẽ quay đầu ngược lại
  */
  if ((left_sensor_state == 1)&&(right_sensor_state ==0)){}
  if ((left_sensor_state == 0)&&(right_sensor_state ==1)){} 
  if ((left_sensor_state == 1)&&(right_sensor_state ==1)){}
  
  
  
  if(distance < stop_distance){} // nếu khoảng cách nhỏ hơn giới hạn 
  



}






// chương trình con: thẳng, trái, phải, lùi


// chương trình con xe robot đi tiến
void forward(){ 
  digitalWrite(motorA1, LOW);
  digitalWrite(motorA2, HIGH);                       
  digitalWrite(motorB1, LOW);
  digitalWrite(motorB2, HIGH);
}

// chương trình con xe robot rẽ phải
void turnRight(){ 
  digitalWrite(motorA1, LOW);
  digitalWrite(motorA2, LOW);                       
  digitalWrite(motorB1, LOW);
  digitalWrite(motorB2, HIGH);
}

// chương trình con xe robot rẽ trái
void turnLeft(){
  digitalWrite(motorA1, LOW);
  digitalWrite(motorA2, HIGH);                       
  digitalWrite(motorB1, LOW);
  digitalWrite(motorB2, LOW);
}

// chương trình con xe robot đi lùi
void goBack(){
  digitalWrite(motorA1, HIGH);
  digitalWrite(motorA2, LOW);                       
  digitalWrite(motorB1, HIGH);
  digitalWrite(motorB2, LOW);
}

// chương trình con xe robot dừng lại
void Stop(){
  digitalWrite(motorA1, LOW);
  digitalWrite(motorA2, LOW);                       
  digitalWrite(motorB1, LOW);
  digitalWrite(motorB2, LOW);
}






 

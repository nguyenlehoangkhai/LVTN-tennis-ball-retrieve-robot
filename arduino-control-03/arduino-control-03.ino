#include <Servo.h>

int distance = 20; // Khoảng cách phát hiện vật cản 12cm
void ultrasonic_up();
void servo_control();

//L298 kết nối arduino
const int motorA1      = 3;  // kết nối chân IN1 với chân 3 arduino
const int motorA2      = 4;  // kết nối chân IN2 với chân 4 arduino
const int motorAspeed  = 5;  // kết nối chân ENA với chân 5 arduino
const int motorB1      = 7; // kết nối chân IN3 với chân 7 arduino
const int motorB2      = 8; // kết nối chân IN4 với chân 8 arduino
const int motorBspeed  = 6;  // kết nối chân ENB với chân 6 arduino
 
//Kết nối cảm biến siêu âm HC-SRF 04 phía trên 
const int trig_up = 11; // kết nối chân trig với chân 11 arduino
const int echo_up = 12; // kết nối chân echo với chân 12 arduino

// kết nối servo và cảm biến siêu âm ở phía dưới
Servo myser;
const int serpin=13;
const int trig_down = 9;     //Chân trig của HC-SR04
const int echo_down = 10;     //Chân echo của HC-SR04
int deg, dis;     //Biến lưu độ và khoảng cách (degree,distance)

//kết nối của 3 cảm biến hồng ngoại và phát hiện quả bóng (dò line )
const int L_S = A0;  // cb dò line trái
const int R_S = A1; // cb dò line phải
const int ball_detect = A2; // cb dò line phải

int left_sensor_state; // biến lưu cảm biến hồng ngoại line trái
int right_sensor_state; // biến lưu cảm biến hồng ngoại line phải
int ball_detect_state;

// Biến lưu thời gian và khoảng cách cho cảm biến siêu âm trên
long up_duration; // biến lưu thời gian
int up_distance;  // biến khoảng cách
    
// Hàm khởi tạo
void setup() {
  pinMode(L_S,INPUT); // chân cảm biến khai báo là đầu vào
  pinMode(R_S,INPUT);
  pinMode(motorA1, OUTPUT); 
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(motorAspeed, OUTPUT);
  pinMode(motorBspeed, OUTPUT);
  pinMode(trig_up, OUTPUT); 
  pinMode(echo_up, INPUT); 
    
  Serial.begin(9600); 
  analogWrite(motorAspeed, 120); // tốc độ động cơ a ban đầu 120 ( 0 - 255)
  analogWrite(motorBspeed, 120);// tốc độ động cơ b ban đầu 120 ( 0 - 255)
  delay(3000);                               

  // servo và cảm biến siêu âm dưới
  myser.attach(serpin);   //Kết nối servo
  
  pinMode(trig_down,OUTPUT);   //Chân trig sẽ phát tín hiệu
  pinMode(echo_down,INPUT);    //Chân echo sẽ nhận tín hiệu
  //  Serial.begin(9600);     //Set Baudrate
  
  }

//Hàm lặp
void loop() {
  ultrasonic_up();
  sensor_ir();
  
  // nếu khoảng cách nhỏ hơn giới hạn 
  if(up_distance < distance){
    turn_180();
    servo_control();
    } 
   
  }



 

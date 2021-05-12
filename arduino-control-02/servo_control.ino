//Thư viện
#include <Servo.h>

//Khai báo servo
  
void servo_control() 

{
 
 //Quay từ 0 độ đến 180 độ
 for (deg=0;deg <=180; deg++)
 {
  myser.write(deg);         //Quay servo đến deg độ
  dis= down_distance();           //dis gán = khoảng cách
  Serial.print(deg);        //Xuất ra serial cho Processing đọc
  Serial.print(",");
  Serial.print(dis);
  Serial.print(".");
  Serial.println("");
  delay(50);
 }

 //Quay từ 180 độ về 0 độ (tương tự ở trên)
  for (deg = 180; deg>=0 ;deg--)
 {
  myser.write(deg);
  dis = down_distance();
  Serial.print(deg);
  Serial.print(",");
  Serial.print(dis);
  Serial.print(".");
  Serial.println("");
  delay(50);
 }
}

int down_distance()
{
    unsigned long down_duration; //Biến đo thời gian
  
    digitalWrite(trig_down, 0);   //Tắt chân trig
    delayMicroseconds(2);   //Chờ 2ms
    digitalWrite(trig_down, 1);   //Phát xung từ chân trig
    delayMicroseconds(5);   //Xung có độ dài 5 microSeconds
    digitalWrite(trig_down, 0);   //Tắt chân trig
    
    down_duration = pulseIn(echo_down,HIGH);  //Đo độ rộng xung HIGH ở chân echo. 
    return int(down_duration/2/29.412);  //Tính khoảng cách đến vật. 
}

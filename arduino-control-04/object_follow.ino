//void object_follow(){
//int leftDistance, rightDistance;
//  for (deg=90;deg <= 120; deg++){
//      myser.write(deg);         //Quay servo đến deg độ
////      dis= down_distance();           //dis gán = khoảng cách
//      Serial.print(deg);        //Xuất ra serial cho Processing đọc
//      Serial.print(",");
//      Serial.print(dis);
//      Serial.print(".");
//      Serial.println("");
//      delay(15);
//      
//      myser.write(120);  //setservo position to right side
//      delay(200); 
////      rightDistance = down_distance();
//      }
//
//  for (deg = 120 ; deg >= 60; deg--){
//      myser.write(deg);
//  //    dis= down_distance();
//      Serial.print(deg);
//      Serial.print(",");
//      Serial.print(dis);
//      Serial.print(".");
//      Serial.println("");
//      delay(15);
//      
//      myser.write(60);  //setservo position to left side
//      delay(200); 
//  //    leftDistance = down_distance();
//      }
//
//  if((rightDistance > 70)&&(leftDistance > 70)){Stop();
//  }else if((rightDistance >= 20) && (leftDistance >= 20)){forward();delay(100);
//  }else if((rightDistance <= 10) && (leftDistance <= 10)){back();delay(100);
//  }else if(rightDistance - 3 > leftDistance){turnLeft();delay(100);
//  }else if(rightDistance + 3 < leftDistance){turnRight();delay(100);
//  }else {Stop();}
//
//}
//    

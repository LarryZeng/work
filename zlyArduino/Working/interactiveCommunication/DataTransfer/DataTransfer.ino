int pinSDA = 5;
int pinSCL = 3;
int pinButton = 4;

byte controlData_record = 0;
word chainData_record = 0;

byte controlData_receive = 0;
word chainData_receive = 0;

void resetState(void)
{
  digitalWrite(pinSDA,HIGH);
  digitalWrite(pinSCL,HIGH);
  pinMode(pinSDA,OUTPUT);
  pinMode(pinSCL,OUTPUT);
}
void setup()
{
  Serial.begin(9600);
  pinMode(pinButton,INPUT);
  
  digitalWrite(pinSCL,HIGH);
  digitalWrite(pinSDA,HIGH);

  pinMode(pinSDA,OUTPUT);
  pinMode(pinSCL,OUTPUT);  
}
char buffer[3];
byte controlData = 0x0b;
word chainData = 0xbd10;

void loop()
{
 if(Serial.available()>0)
 {
    Serial.readBytes(buffer,3);
    Serial.print("buffer");
    Serial.println(buffer);
    
    Serial.println(controlData,BIN);
    Serial.println(chainData,BIN);
    if(handShake_Send() == true)
    {
      sendData(controlData,chainData);
    }
 }
   pinMode(pinSDA,INPUT);
   
   if(digitalRead(pinSDA) == LOW)
   {
     if(handShake_Receive() == true)
     {
       if(receiveData() == true)
       {
    //     Serial.println("controlData");
     //    Serial.println(controlData_receive,BIN);
     //    Serial.println("chainData");
      //   Serial.println(chainData_receive,BIN);
       }
     }
   }
}

boolean handShake_Receive(void)
{
  unsigned long startTime = 0;
  unsigned long stopTime = 0;
  unsigned long gapTime = 0;
  
  pinMode(pinSCL,OUTPUT);
  pinMode(pinSDA,INPUT);

  delayMicroseconds(1000);
  digitalWrite(pinSCL,LOW);  
  
  startTime = micros();
  while ((digitalRead(pinSDA)) == LOW);
  {
    stopTime = micros();
    gapTime = stopTime - startTime;
    if(gapTime > 10000)
    {
      resetState();
      return false;
    }
  }
  delayMicroseconds(1000);
  digitalWrite(pinSCL,HIGH); 
  return true;
}

boolean handShake_Send(void)
{
  unsigned long startTime = 0;
  unsigned long stopTime = 0;
  unsigned long gapTime = 0;

  pinMode(pinSDA,OUTPUT);
  pinMode(pinSCL,INPUT);

  digitalWrite(pinSDA,LOW); 
  startTime = micros();
  while(digitalRead(pinSCL) == HIGH)//wait scl to low
  {
    stopTime = micros();
    gapTime = stopTime - startTime;
    if(gapTime>10000)
    {
     resetState();
     return false;
    }
  }
  delayMicroseconds(1000);
  digitalWrite(pinSDA,HIGH);
 
  startTime = micros();
  while(digitalRead(pinSCL) == LOW)
  {
    stopTime = micros();
    gapTime = stopTime - startTime;
   if(gapTime>10000)
    {
     resetState();
     return false;
    }
  }
  return true;
}
void sendTxsign(boolean sign)
{
  digitalWrite(pinSCL,LOW);
  delayMicroseconds(1000);
  if(sign)
    digitalWrite(pinSDA,HIGH);
  else
    digitalWrite(pinSDA,LOW);
  delayMicroseconds(1000);
  digitalWrite(pinSCL,HIGH);
  delayMicroseconds(1000);
}

void sendChecksum(byte data)
{
  byte temp = 0x80;
  data = data << 4;
  for(int i = 0;i < 4;i++)
  {
    digitalWrite(pinSCL,LOW); //pinSCL = 0;
    delayMicroseconds(1000);
    if((data&temp)!= 0)
    {
      digitalWrite(pinSDA,HIGH); //pinSDA == 1;
    }
    else
    {
      digitalWrite(pinSDA,LOW);// pinSDA = 0;
    }
    temp >>= 1;
    delayMicroseconds(1000);
    digitalWrite(pinSCL,HIGH);
    delayMicroseconds(1000);
  }
}

void sendControlData(byte data)
{
  byte temp = 0x80;
  data = data << 4;
  for(int i = 0;i < 4;i++)
  {
    digitalWrite(pinSCL,LOW); //pinSCL = 0;
    delayMicroseconds(1000);
    if((data&temp)!= 0)
    {
      digitalWrite(pinSDA,HIGH); //pinSDA == 1;
    }
    else
    {
      digitalWrite(pinSDA,LOW);// pinSDA = 0;
    }
    temp >>= 1;
    delayMicroseconds(1000);
    digitalWrite(pinSCL,HIGH);
    delayMicroseconds(1000);
  }
}

void sendChain(word data)
{
  word temp = 0x8000;
  for(int i = 0;i < 16;i++)
  {
    digitalWrite(pinSCL,LOW); //pinSCL = 0;
    delayMicroseconds(1000);
    if((data&temp)!= 0)
    {
      digitalWrite(pinSDA,HIGH); //pinSDA == 1;
    }
    else
    {
      digitalWrite(pinSDA,LOW);// pinSDA = 0;
    }
    temp >>= 1;
    delayMicroseconds(1000);
    digitalWrite(pinSCL,HIGH);
    delayMicroseconds(1000);
  }
}
byte calculateChecksum(byte controlData,word chainData)
{
  byte data = 0x00;
  for(int i=0;i<4;i++)
  {
    data = chainData & 0x0F +data;
    chainData = chainData >> 4;
  }
  data = controlData + data;
  data = data&0x0F;
  return data;
}

void sendData(byte controlData , word chainData)
{
  byte Txsign = 0;
  byte checkSum = 0x00;
  unsigned long startTime = 0;
  unsigned long stopTime = 0;
  static byte count = 0;
  controlData_record = controlData;
  chainData_record = chainData;
  checkSum = calculateChecksum(controlData,chainData);
 
  digitalWrite(pinSCL,HIGH);
  digitalWrite(pinSDA,HIGH);
  pinMode(pinSCL,OUTPUT);
  pinMode(pinSDA,OUTPUT);
  delayMicroseconds(1000);  

  sendTxsign(Txsign);  //send resetSign bit;
  sendChecksum(checkSum); //send checksum;
  sendControlData(controlData);//send controlData;
  sendChain(chainData);    //send chainData;  
  
  digitalWrite(pinSCL,LOW); //end 
  pinMode(pinSDA,INPUT);
  
  delayMicroseconds(500);
  startTime = micros();
  while((digitalRead(pinSDA)) == HIGH)
  {
    stopTime = micros();
    if((stopTime - startTime) > 10000)
    {
      if(count < 3)
      {
        count++;
        sendData(controlData_record,chainData_record);
      }
      resetState();
      return;
    }
  }
  Serial.println("success");
  delayMicroseconds(1000);
  resetState();
}

boolean receiveData(void)
{
  unsigned long data = 0x00000000;
  unsigned long temp = 0x00000000;
  unsigned long startTime = 0;
  unsigned long stopTime = 0;
  unsigned long gapTime = 0;
  
  byte Txsign = 0x00;
  byte checkSum = 0x00;
  byte controlData = 0x00;
  word chainData = 0x00;
  static byte receive_count = 0;
  pinMode(pinSDA,INPUT);
  pinMode(pinSCL,INPUT);
  
  startTime = micros(); 
  while((digitalRead(pinSCL)) == HIGH);
  {
    stopTime = micros();
    if((stopTime - startTime)>10000)
    {
      resetState();
      return false;
    }
  }
 
  for(int i = 0;i<25;i++)
  {
    data = data << 1;
    startTime = micros();
    while((digitalRead(pinSCL)) == LOW)
    {
      stopTime = micros();
      if((stopTime - startTime) > 10000)
      {
        resetState();
        return false;
      }
    }
    if((digitalRead(pinSDA))== HIGH)
    {
      data = data|0x00000001;
      
    }
    startTime = micros();
    while(digitalRead(pinSCL) == HIGH)
    {
      stopTime = micros();
      if((stopTime - startTime) > 10000)
      {
        resetState();
        return false;
      }
    }
  }
  
  Txsign = byte((data & 0x01000000)>>24);
  checkSum = byte((data & 0x00f00000)>>20);
  controlData = byte((data & 0x000f0000)>>16);
  chainData = word((data & 0x0000ffff));

  digitalWrite(pinSDA,HIGH);
  pinMode(pinSDA,OUTPUT);
  delayMicroseconds(1000);
  
  int testnum = calculateChecksum(controlData,chainData);
  if(checkSum == testnum)
  {
    digitalWrite(pinSDA,LOW);
  }
  else
  {
    resetState(); 
    return false;
  }
  controlData_receive = controlData;
  chainData_receive = chainData;
  delayMicroseconds(1000);
  resetState();            
  return true;
}



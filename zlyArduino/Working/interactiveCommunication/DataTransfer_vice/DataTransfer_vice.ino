#define MAX_WAIT_TIME 10000
#define RX_BUF_SIZE 100
#define MAX_RESEND_COUNT 5
#define FAIL_WAIT_TIME 500
int pinSDA = 2;
int pinSCL = 3;
int pinButton = 4;

byte controlData_record = 0;
word chainData_record = 0;

byte controlData_receive = 0;
word chainData_receive = 0;

char rx_buf[RX_BUF_SIZE];
int rx_buf_cnt;

byte controlData = 0;
word chainData = 0;


unsigned long getGapTime(unsigned long startTime,unsigned long stopTime)
{
  unsigned long gapTime = 0;
  const unsigned long resetTime = 4294967295ul;
  if((stopTime - startTime)< 0)
  {
    gapTime = resetTime - startTime + stopTime; //4294967296 is reset time
  }
  else
  {
    gapTime = stopTime- startTime;
  }
  return gapTime;
}

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
}

void loop()
{
  int reSendCount = 0;
  while(Serial.available()>0)
  {
    char ch = Serial.read();
    rx_buf[rx_buf_cnt] = ch;
    if(rx_buf_cnt < RX_BUF_SIZE)
    {
      rx_buf_cnt++;
    }
    if(ch == 0x0D)
    {
      controlData = rx_buf[0]&0x0F;
      word temp = rx_buf[1];
      chainData = (temp<<8)|rx_buf[2];

      Serial.println(controlData,BIN);
      Serial.println(chainData,BIN);
      while(reSendCount < MAX_RESEND_COUNT)
      {
         Serial.println("ready send data");
         Serial.println(reSendCount);
        if((sendData(controlData,chainData)) == true)
        {
          break;
        }
        delay(FAIL_WAIT_TIME);
        reSendCount++;
      }
      rx_buf_cnt = 0;
    }
  }

  pinMode(pinSDA,INPUT);   
  if(digitalRead(pinSDA) == LOW)
  {
    if(handShake_Receive() == true)
    {
      if(receiveData() == true)
      {
         Serial.println("controlData");
         Serial.println(controlData_receive,BIN);
         Serial.println("chainData");
         Serial.println(chainData_receive,BIN);
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
    gapTime = getGapTime(startTime,stopTime);
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
    gapTime = getGapTime(startTime,stopTime);
    if(gapTime > 10000)
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
    gapTime = getGapTime(startTime,stopTime);
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

boolean sendData(byte controlData , word chainData)
{
  byte Txsign = 0;
  byte checkSum = 0x00;
  unsigned long startTime = 0;
  unsigned long stopTime = 0;
  unsigned long gapTime = 0;
  static byte count = 0;
  controlData_record = controlData;
  chainData_record = chainData;
  checkSum = calculateChecksum(controlData,chainData);
 
  if(handShake_Send() == false)
  {
    Serial.println("A");
    return false;
  }
 
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
    gapTime = getGapTime(startTime,stopTime);
    if(gapTime > 10000)
    {
      Serial.println("send fail");
      return false;
    }
  }
  delayMicroseconds(1000);
  resetState();
  delay(250);
  return true;
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
    gapTime = getGapTime(startTime,stopTime);
    if(gapTime > 10000)
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
      gapTime = getGapTime(startTime,stopTime);
      if(gapTime > 10000)
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
      gapTime = getGapTime(startTime,stopTime);
      if(gapTime > 10000)
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



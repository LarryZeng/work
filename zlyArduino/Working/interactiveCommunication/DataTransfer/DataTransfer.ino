#define codeElementTime 1000
#define delayGapTime 1000
#define RX_BUF_SIZE 100
#define MAX_RESEND_COUNT 5
#define FAIL_WAIT_TIME 500
#define DELAY_TIME 1000
int pinSDA = 2;
int pinSCL = 3;

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
  pinMode(pinSDA,INPUT_PULLUP);
  pinMode(pinSCL,INPUT_PULLUP);
}
void setup()
{
  Serial.begin(9600);
  pinMode(pinSDA,INPUT_PULLUP);
  pinMode(pinSCL,INPUT_PULLUP);
}

void loop()
{
  int reSendCount = 0;
  while(Serial.available()>0)   //如果串口有数据进来,则返回数据大于零
  {
    char ch = Serial.read();
    rx_buf[rx_buf_cnt] = ch;
    if(rx_buf_cnt < RX_BUF_SIZE)
    {
      rx_buf_cnt++;
    }
    if(ch == 0x0D || ch == 0x0A) //如果读到回车或者换行就发送数据
    {
      if(rx_buf_cnt > 3)        //如果串口传入的字符大于等于四个则发送数据，反之则不发送数据
      {
        controlData = rx_buf[0]&0x0F;
        word temp = rx_buf[1];
        chainData = (temp<<8)|rx_buf[2];

       // Serial.println(controlData,BIN);
       // Serial.println(chainData,BIN);
        while(reSendCount < MAX_RESEND_COUNT) //失败重发次数小于5次就可以继续重发
        {
          Serial.println("ready send data");
          Serial.println(reSendCount);
         // controlData = 0x0f;
        //  chainData = 0x8ff1;
          Serial.println(controlData,HEX);
          Serial.println(chainData,HEX);
          if((sendData(controlData,chainData)) == true) //如果发送成功则退出循环，否则就继续重发
          {
            break;
          }
          delay(FAIL_WAIT_TIME);
          reSendCount++;
        }
      }
      rx_buf_cnt = 0;
    }
  }
  if(digitalRead(pinSDA) == LOW)  //读到sda为低电平，表示另一边又数据过来
  { 
    if(handShake_Receive() == true) //如果握手成功则开始接收数据
    {
      if(receiveData() == true) //经过校验接收的数据正确
      {
       // Serial.println(chainData,HEX);
        Serial.println(controlData_receive,HEX);
        Serial.println(chainData_receive,HEX);
     //   printDataWithHex(chainData_receive); //以16进制格式打印出接收到的数据
        if(controlData_receive == 7)  //通过接收到的数据的个数来分隔数据
        Serial.println();
      }
    }
  }   
  
}
void printDataWithHex(word data)
{
  byte temp = 0;
  for(int i = 0;i<4;i++)
 {
    temp = (data & 0xf000)>>12;
    data = data << 4;
    Serial.print(temp,HEX);

 } 
     Serial.print(" ");
}

boolean handShake_Receive(void)
{
  unsigned long startTime = 0;
  unsigned long stopTime = 0;
  unsigned long gapTime = 0;
  
  delayMicroseconds(delayGapTime);
  pinMode(pinSCL,OUTPUT);  
  digitalWrite(pinSCL,LOW);
  
  startTime = micros();
  while ((digitalRead(pinSDA)) == LOW);
  {
    stopTime = micros();
    gapTime = getGapTime(startTime,stopTime);
    if(gapTime > (3*codeElementTime))
    {
      Serial.println("Error,wait time too long");
      Serial.println(gapTime);
      resetState();
      return false;
    }
  }
 
  delayMicroseconds(1000);//modify by zly
  digitalWrite(pinSCL,HIGH); 
  
  Serial.println("receive data hand success");
  return true;
}

boolean handShake_Send(void)
{
  unsigned long startTime = 0;
  unsigned long stopTime = 0;
  unsigned long gapTime = 0;

  pinMode(pinSDA,OUTPUT);
  digitalWrite(pinSDA,LOW); 
  startTime = micros();
  while(digitalRead(pinSCL) == HIGH)//wait scl to low
  {
    stopTime = micros();
    gapTime = getGapTime(startTime,stopTime);
    if(gapTime > (5*codeElementTime))
    {
     resetState();
     return false;
    }
  }
  delayMicroseconds(delayGapTime);
  digitalWrite(pinSDA,HIGH);
 
  startTime = micros();
  while(digitalRead(pinSCL) == LOW)
  {
    stopTime = micros();
    gapTime = getGapTime(startTime,stopTime);
   if(gapTime > (3*codeElementTime))
    {
     resetState();
     return false;
    }
  }
  Serial.println("Send data hand success");
  return true;
}
void sendTxsign(boolean sign)  //发送重发标记
{
  digitalWrite(pinSCL,LOW);     
  delayMicroseconds(delayGapTime);
  if(sign)                         //在低电平状态的时候发送数据
  {
    digitalWrite(pinSDA,HIGH);
  }
  else
  {
    digitalWrite(pinSDA,LOW);
  }
  delayMicroseconds(delayGapTime);
 // pinMode(pinSCL,INPUT);
  digitalWrite(pinSCL,HIGH);
  delayMicroseconds(2*delayGapTime);
}

void sendChecksum(byte data)  //发送校验和
{
  byte temp = 0x80;
  data = data << 4;
  for(int i = 0;i < 4;i++)
  {
    digitalWrite(pinSCL,LOW); //pinSCL = 0;
    delayMicroseconds(delayGapTime);
    if((data&temp)!= 0)
    {
      digitalWrite(pinSDA,HIGH); //pinSDA == 1;
    }
    else
    {
      digitalWrite(pinSDA,LOW);// pinSDA = 0;
    }
    temp >>= 1;
    delayMicroseconds(delayGapTime);
    digitalWrite(pinSCL,HIGH);
    if(i == 3)
      delayMicroseconds(delayGapTime);
    else
      delayMicroseconds(2*delayGapTime);
  }
}

void sendControlData(byte data)
{
  byte temp = 0x80;
  data = data << 4;
  for(int i = 0;i < 4;i++)
  {
   // pinMode(pinSCL,OUTPUT);
    digitalWrite(pinSCL,LOW); //pinSCL = 0;
    delayMicroseconds(delayGapTime);
    if((data&temp)!= 0)
    {
     // pinMode(pinSDA,INPUT);
      digitalWrite(pinSDA,HIGH); //pinSDA == 1;
    }
    else
    {
      //pinMode(pinSDA,OUTPUT);
      digitalWrite(pinSDA,LOW);// pinSDA = 0;
    }
    temp >>= 1;
    delayMicroseconds(delayGapTime);
    //pinMode(pinSCL,INPUT);
    digitalWrite(pinSCL,HIGH);
    delayMicroseconds(2*delayGapTime);
  }
}

void sendChain(word data)
{
  word temp = 0x8000;
  for(int i = 0;i < 16;i++)
  {
   // pinMode(pinSCL,OUTPUT);
    digitalWrite(pinSCL,LOW); //pinSCL = 0;
    delayMicroseconds(delayGapTime);
    if((data&temp)!= 0)
    {
    //  pinMode(pinSDA,INPUT);
      digitalWrite(pinSDA,HIGH); //pinSDA == 1;
    }
    else
    {
    //  pinMode(pinSDA,OUTPUT);
      digitalWrite(pinSDA,LOW);// pinSDA = 0;
    }
    temp >>= 1;
    delayMicroseconds(delayGapTime);
//    pinMode(pinSCL,INPUT);
    digitalWrite(pinSCL,HIGH);
    delayMicroseconds(2*delayGapTime);
  }
}
byte calculateChecksum(byte controlData,word chainData)
{
  byte data = 0x00;
  for(int i=0;i<4;i++)
  {
    data = (chainData & 0x0F) +data;
    chainData = chainData >> 4;
  }

  data = controlData + data;
  data = data&0x0F;
  byte temp = 0;
  byte num = 0;
  for(int j = 0;j<4;j++)   //取反
  {
    temp = data & 0x01;
    num = num << 1;
    num = num | temp;
    data = data >> 1;
  }
  return num;
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
  checkSum = calculateChecksum(controlData,chainData); //计算校验和
 
  if(handShake_Send() == false)
  {
    Serial.println("handShake fail,wait time too long");
    return false;
  }
  
 // Serial.print(Txsign,BIN);
//  Serial.print(controlData,BIN);
 // Serial.print(chainData,BIN);
 // Serial.println(checkSum,BIN);
  
  Serial.print("checksum:");
  Serial.println(checkSum,BIN);
          
  digitalWrite(pinSDA,HIGH);
  digitalWrite(pinSCL,HIGH);
  pinMode(pinSDA,OUTPUT);
  pinMode(pinSCL,OUTPUT);
  
  delayMicroseconds(delayGapTime);
  
  sendTxsign(Txsign);  //send resetSign bit;
  sendControlData(controlData);//send controlData;
  sendChain(chainData);    //send chainData;  
  sendChecksum(checkSum); //send checksum;
   
  pinMode(pinSDA,INPUT_PULLUP); 
  delayMicroseconds(delayGapTime);
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
  delayMicroseconds(delayGapTime);
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
  pinMode(pinSDA,INPUT_PULLUP);
  pinMode(pinSCL,INPUT_PULLUP);
  
  startTime = micros(); 
  while((digitalRead(pinSCL)) == HIGH);
  {
    stopTime = micros();
    gapTime = getGapTime(startTime,stopTime);
    if(gapTime > (5*codeElementTime))
    {
      Serial.println("time too long");
      resetState();
      return false;
    }
  }
  Serial.println("beginreceive");
  for(int i = 0;i<25;i++)
  {
    data = data << 1;
    startTime = micros();
    while((digitalRead(pinSCL)) == LOW)
    {
      stopTime = micros();
      gapTime = getGapTime(startTime,stopTime);
      if(gapTime > 5*codeElementTime)
      {
        Serial.println("ERROR1");
        Serial.println(gapTime);
        resetState();
        return false;
      }
    }
    if((digitalRead(pinSDA))== HIGH)
    {
      data = data|0x00000001;
    }
    startTime = micros();
    while((digitalRead(pinSCL) == HIGH) && i != 24)
    {
      stopTime = micros();
      gapTime = getGapTime(startTime,stopTime);
      if(gapTime > 5*codeElementTime)
      {
        Serial.println("ERROR2");
        Serial.println(gapTime);
        resetState();
        return false;
      }
    }
  }

  Txsign = byte((data & 0x01000000)>>24);
  controlData = byte((data & 0x00f00000)>>20);
  chainData = word((data & 0x000ffff0)>>4);
  checkSum = byte(data & 0x0000000f);

  delayMicroseconds(delayGapTime);
  int testnum = calculateChecksum(controlData,chainData);
  
  digitalWrite(pinSDA,HIGH);
  pinMode(pinSDA,OUTPUT);
  
  if(checkSum == testnum)
  {
    digitalWrite(pinSDA,LOW);
  }
  else
  {
    Serial.println("checkError");
    resetState(); 
    return false;
  }
  controlData_receive = controlData;
  chainData_receive = chainData;
  delayMicroseconds(DELAY_TIME);
  resetState();            
  return true;
}



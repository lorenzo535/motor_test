
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 15
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
unsigned long tag_on_sensor_time, previous_tag_on_sensor_time;

//motor A connected between A01 and A02
//motor B connected between B01 and B02
#define POS_MIN 20   //LOCK position
#define POS_MAX 450 //UNLOCK position

#define OUT_LIMIT POS_MIN
#define IN_LIMIT POS_MAX
#define RANGE POS_MAX - POS_MIN
#define MARGIN 5 //RANGE/10

#define ACTUATOR_SPEED 255

#define DIRECTION_OUT  1
#define DIRECTION_IN   0

#define STATE_STOPPED  1
#define STATE_CLOSED   2
#define STATE_RUNNING  3
#define STATE_OPENED   4

int STBY = 7; //standby
int current_state;
int button_old;
#define BUTTON_IN 2
long int time_started;
long int delta;
#define MAX_RUNNING_TIME_MS 1000
#define KEEP_CLUTCH_ON 0

////////////////   KEYS DEFINITION //////

String keys_table[]  = {
  "55 79 0B AB",
  "FA 1A 90 AB",
  "41 8E DC 2B",
  "B3 2E DC 2B",
  "70 72 DC 2B",
  "FA 62 DC 2B"
};

#define KEYTABLESIZE 6 
////////////////   END KEYS DEFINITION //////


//Motor A
int PWMA = 3; //Speed control 
int AIN1 = 9; //Direction
int AIN2 = 8; //Direction

//Motor B
int PWMB = 5; //Speed control
int BIN1 = 4; //Direction
int BIN2 = 6; //Direction

int position_in = A6;
int position = 0;
int command = DIRECTION_OUT;

void move(int motor, int speed, int direction);
boolean limits_ok(int direction);

void setup(){
  
  pinMode (BUTTON_IN, INPUT);
  pinMode(STBY, OUTPUT);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  
  Serial.begin(9600);
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  current_state= STATE_STOPPED;
  previous_tag_on_sensor_time = 0;
  button_old = 0;
}

void loop(){


//Serial.print(digitalRead(BUTTON_IN));
//Serial.println();

RunProgram( DetectTag()|ButtonPressed() );
//limits_ok(0);
    
return;
}


boolean ButtonPressed()
{
   int  button = digitalRead(BUTTON_IN);
  if ((!button_old) && button)
  {
    Serial.print ("pressed");
    Serial.println();
    button_old = button;
    return true;
    
  }
  button_old = button;
  return false;
  
}


boolean DetectTag()
{

if ( mfrc522.PICC_IsNewCardPresent()) 
  {
    mfrc522.PICC_ReadCardSerial();
 
    //Show UID on serial monitor
    Serial.print("UID tag :");
    String content= "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
       Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
       Serial.print(mfrc522.uid.uidByte[i], HEX);
       content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
       content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    //Serial.print("Message : ");
    content.toUpperCase();
    if (KeyIsKnown(content.substring(1)))
    {              
        tag_on_sensor_time = millis();     
    }
  
  }

  else {
   previous_tag_on_sensor_time  = tag_on_sensor_time;
   return false;
  }
  
  unsigned long delta = tag_on_sensor_time - previous_tag_on_sensor_time;
if (delta > 100)
{
  Serial.println();
    Serial.print("CLICK ");
   previous_tag_on_sensor_time  = tag_on_sensor_time;
  return true;
}
 
  return false;
}


void RunProgram(bool click)
{

  //keep clutch on
  //if (KEEP_CLUTCH_ON)
   // move(2, 255, 1); //motor 2, full speed, left

  switch (current_state)
  {
    case STATE_STOPPED :
    //digitalWrite(STBY, LOW); //disable motors
    //clutch off
    move(2, 255, 0); //motor 2, full speed, left
    move(1, 0, 1); //motor 1, stop, 
    if (click)    
    {
      current_state= STATE_RUNNING;    
      command = !command;
      time_started = millis();     
      
    }

    break;

    case STATE_RUNNING :

      //clutch on
      move(2, 255, 1); //motor 2, full speed, left
      //timeout      
      delta  = millis()-time_started;
      //Serial.print("delta :");
      //Serial.println(delta);
      if ( delta >= MAX_RUNNING_TIME_MS) 
      {        
        current_state= STATE_STOPPED; 
        break;
      }  
          
      if (!limits_ok(command))
      {
      //command = !command;
      move(1, 0, 1); //motor 1, stop, 
      current_state= STATE_STOPPED;      
      }else
        move(1, ACTUATOR_SPEED, command); //motor 1, half speed, 

    break;
  
  }
  
    
}

bool KeyIsKnown (String received_key)
{

  int i = 0;

  for (i = 0; i < KEYTABLESIZE; i++)
    if (received_key == keys_table[i])
      return true;

  return false;
  
}
  
/* 
int StateEvaluate()
{

    position = analogRead(A6);
    Serial.println (position);

    if (!motor_running)
    {
      if (position  <= POS_MIN)
        return STATE_CLOSED;
      else if (position  >= POS_MAX)
        return STATE_OPENED;
      else
        return STATE_STOPPED;
    }
    else
    return  STATE_RUNNING;
  
}
*/

void move(int motor, int speed, int direction){
//Move specific motor at speed and direction
//motor: 0 for B 1 for A
//speed: 0 is off, and 255 is full speed
//direction: 0 clockwise, 1 counter-clockwise

  digitalWrite(STBY, HIGH); //disable standby

  boolean inPin1 = LOW;
  boolean inPin2 = HIGH;

  if(direction == 1){
    inPin1 = HIGH;
    inPin2 = LOW;
  }

  if(motor == 1){
    
    digitalWrite(AIN1, inPin1);
    digitalWrite(AIN2, inPin2);
    analogWrite(PWMA, speed);

  }else{
    digitalWrite(BIN1, inPin1);
    digitalWrite(BIN2, inPin2);
    analogWrite(PWMB, speed);
  }
}

void stop(){
//enable standby  
  digitalWrite(STBY, LOW); 
}

boolean limits_ok(int direction)
{

    position = analogRead(A6);
    Serial.print ("position:");
    Serial.println (position);
    
    if ((direction ==1)&& (position <= POS_MIN))
    {  Serial.println("Lower limit STOP");
      return false;
    }
    if ((direction ==0)&& (position >= POS_MAX))
    {
       Serial.println("Higher limit STOP");
      return false;
    }      
    return true;         
}

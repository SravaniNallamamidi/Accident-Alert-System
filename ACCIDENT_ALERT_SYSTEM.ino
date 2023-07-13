#include<PulseSensorPlayground.h>
#include <AltSoftSerial.h>    //GPS_MODULE
#include <TinyGPS++.h>
#include <SoftwareSerial.h> //GSM_MODULE
#include <math.h>
#include<Wire.h>
int sen1=A0;      //speedometer input
int Redled=9;     //speed alert
unsigned long start_time=0;
unsigned long end_time=0;
unsigned long impact_time;
//unsigned long time1;
unsigned long alert_delay = 30000;
float velocity,dif,velocityreal,starttime,endtime;
float speedconstant=7.5;
int pulsewire=6;
int threshold=550;
PulseSensorPlayground pulsesensor;
int vib_pin=A5;
int butt_pin=8;
int buz_pin=7;
int vibrationsensor;
int buttonstate=0;
const String EMERGENCY_PHONE = "+919666468505";
//------GSM_MODULE--------
int gsmrxpin=2;
int gsmtxpin=3;
SoftwareSerial sim800(gsmrxpin,gsmtxpin);
//GPS Module----------
int gpsrxpin=4;
int gpstxpin=5;
AltSoftSerial neogps;
TinyGPSPlus gps;
//--------------------------------------------------------------
String sms_status,sender_number,received_date,msg;
String latitude, longitude;



void setup() {
  Serial.begin(9600);

  //Serial.println("SIM800L serial initialize");
  sim800.begin(9600);
 
  //Serial.println("NEO6M serial initialize");
  neogps.begin(9600);

  pinMode(sen1,INPUT);
  analogWrite(9,LOW);
  pulsesensor.analogInput(pulsewire);
  pulsesensor.setThreshold(threshold);
  pinMode(vib_pin,INPUT);
  pinMode(butt_pin,INPUT);
  pinMode(buz_pin,OUTPUT);
  //Scheduler.startLoop(loop2);
  sms_status = "";
  sender_number="";
  received_date="";
  msg="";

  //--------------------------------------------------------------
  sim800.println("AT"); //Check GSM Module
  delay(1000);
  //SendAT("AT", "OK", 2000); //Check GSM Module
  sim800.println("ATE1"); //Echo ON
  delay(1000);
  //SendAT("ATE1", "OK", 2000); //Echo ON
  sim800.println("AT+CPIN?"); //Check SIM ready
  delay(1000);
  //SendAT("AT+CPIN?", "READY", 2000); //Check SIM ready
  sim800.println("AT+CMGF=1"); //SMS text mode
  delay(1000);
  //SendAT("AT+CMGF=1", "OK", 2000); //SMS text mode
  sim800.println("AT+CNMI=1,1,0,0,0"); /// Decides how newly arrived SMS should be handled
  delay(1000);
  //SendAT("AT+CNMI=1,1,0,0,0", "OK", 2000); //set sms received format
  //AT +CNMI = 2,1,0,0,0 - AT +CNMI = 2,2,0,0,0 (both are same)
   //time1 = micros();
}

void speedsensor()
{
  if (analogRead(sen1) < 500)
  {
    starttime=millis();
    digitalWrite(Redled,LOW);
    delay(30);
  }
  //if(analogRead(sen1) > 500
  else
  {
    endtime=millis();
    dif=endtime-starttime;
    velocity=speedconstant/dif;
    velocityreal=(velocity*360)/100;
    delay(30);
    digitalWrite(Redled,HIGH);
    Serial.print("\n The Velocity is:\n");
    Serial.println(velocityreal);
    Serial.print("km/hr");
  }
}
void loop()
{
  speedsensor();
  vibrationsensor=digitalRead(A5);
  buttonstate=digitalRead(butt_pin);
  impact_time=millis();
  if (vibrationsensor==1)
  {
    
    if (buttonstate==LOW)
    {
      Serial.println("Impact detected!!");
      digitalWrite(buz_pin,HIGH);
      delay(1000);
      digitalWrite(buz_pin,LOW);
      makeCall();
      delay(1000);
      sendAlert();
      
      int BPM=pulsesensor.getBeatsPerMinute();
      if (pulsesensor.sawStartOfBeat())
           {
                 Serial.print("\n HEART BEAT\n");
                 Serial.print("BPM:");
                 Serial.println(BPM);
           }
        delay(50);

      getGps();
        
    }
    while(sim800.available())
    {
      parseData(sim800.readString());
     }
    while(Serial.available()) 
    {
    sim800.println(Serial.readString());
    }
     
  }
  else
  {
    Serial.print("\n  ACCIDENT NOT DETECTED\n");
  }
}
void parseData(String buff){
  Serial.println(buff);

  unsigned int len, index;
  
  //Remove sent "AT Command" from the response string.
  index = buff.indexOf("\r");
  buff.remove(0, index+2);
  buff.trim();
  //------------------
  if(buff != "OK"){
    
    index = buff.indexOf(":");
    String cmd = buff.substring(0, index);
    cmd.trim();
    
    buff.remove(0, index+2);
    //Serial.println(buff);
    //--------------------------------------------------------------
    if(cmd == "+CMTI"){
      //get newly arrived memory location and store it in temp
      //temp = 4
      index = buff.indexOf(",");
      String temp = buff.substring(index+1, buff.length()); 
      temp = "AT+CMGR=" + temp + "\r"; 
      //AT+CMGR=4 i.e. get message stored at memory location 4
      sim800.println(temp); 
    }
    //--------------------------------------------------------------
    else if(cmd == "+CMGR"){
      //extractSms(buff);
      //Serial.println(buff.indexOf(EMERGENCY_PHONE));
      if(buff.indexOf(EMERGENCY_PHONE) > 1){
        buff.toLowerCase();
        //Serial.println(buff.indexOf("get gps"));
        if(buff.indexOf("get gps") > 1){
          getGps();
          String sms_data;
          sms_data = "GPS Location Data\r";
          sms_data += "http://maps.google.com/maps?q=loc:";
          sms_data += latitude + "," + longitude;
        
          sendSms(sms_data);
        }
      }
    }
    //--------------------------------------------------------------
  }
  else{
  //The result of AT Command is "OK"
  }
}
void getGps()
{
  // Can take up to 60 seconds
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (neogps.available()){
      if (gps.encode(neogps.read())){
        newData = true;
        break;
      }
    }
  }
  
  if (newData) //If newData is true
  {
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);
    newData = false;
  }
  else {
    Serial.println("No GPS data is available");
    latitude = "";
    longitude = "";
  }

  Serial.print("Latitude= "); Serial.println(latitude);
  Serial.print("Lngitude= "); Serial.println(longitude);
}

void sendAlert()
{
  String sms_data;
  sms_data = "Accident Alert!!\r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += latitude + "," + longitude;

  sendSms(sms_data);
}

void makeCall()
{
  Serial.println("calling....");
  sim800.println("ATD"+EMERGENCY_PHONE+";");
  delay(20000); //20 sec delay
  sim800.println("ATH");
  delay(1000); //1 sec delay
}

 void sendSms(String text)
{
  //return;
  sim800.print("AT+CMGF=1\r");
  delay(1000);
  sim800.print("AT+CMGS=\""+EMERGENCY_PHONE+"\"\r");
  delay(1000);
  sim800.print(text);
  delay(100);
  sim800.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
  delay(1000);
  Serial.println("SMS Sent Successfully.");
}

boolean SendAT(String at_command, String expected_answer, unsigned int timeout){

    uint8_t x=0;
    boolean answer=0;
    String response;
    unsigned long previous;
    
    //Clean the input buffer
    while( sim800.available() > 0) sim800.read();

    sim800.println(at_command);
    
    x = 0;
    previous = millis();

    //this loop waits for the answer with time out
    do{
        //if there are data in the UART input buffer, reads it and checks for the asnwer
        if(sim800.available() != 0){
            response += sim800.read();
            x++;
            // check if the desired answer (OK) is in the response of the module
            if(response.indexOf(expected_answer) > 0){
                answer = 1;
                break;
            }
        }
    }while((answer == 0) && ((millis() - previous) < timeout));

  Serial.println(response);
  return answer;
}

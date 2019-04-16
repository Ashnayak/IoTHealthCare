#include <SoftwareSerial.h>       //Software Serial library
SoftwareSerial espSerial(4, 5);   //Pin 2 and 3 act as RX and TX. Connect them to TX and RX of ESP8266      
#define DEBUG true

int sensor_pin = A1;                
volatile int heart_rate;          
volatile int analog_data;              
volatile int time_between_beats = 600;            
volatile boolean pulse_signal = false;    

volatile int beat[10];         //heartbeat values will be sotred in this array    

volatile int peak_value = 512;          
volatile int trough_value = 512;        
volatile int thresh = 525;              
volatile int amplitude = 100;                 
volatile boolean first_heartpulse = true;      
volatile boolean second_heartpulse = false;    

volatile unsigned long samplecounter = 0;   //This counter will tell us the pulse timing

volatile unsigned long lastBeatTime = 0;

float temp;
int tempPin = A0;
String mySSID = "Wolfie";      		 // WiFi SSID
String myPWD = "12345677"; 		 // WiFi Password
String myAPI = "RX5SSW7ZBVE8GW5M";       // API Key
String myHOST = "api.thingspeak.com";
String myPORT = "80";
String myFIELD = "field1"; 

String myFIELD2 = "field2";

int sendVal;
int sendVal2;


void setup()
{
  Serial.begin(9600);
  
  interruptSetup();
  
  espSerial.begin(115200);
  
  espData("AT+RST", 1000, DEBUG);                      //Reset the ESP8266 module
  espData("AT+CWMODE=1", 1000, DEBUG);                 //Set the ESP mode as station mode
  espData("AT+CWJAP=\""+ mySSID +"\",\""+ myPWD +"\"", 1000, DEBUG);   //Connect to WiFi network
  /*while(!esp.find("OK")) 
  {          
      //Wait for connection
  }*/
  delay(5000);
  
}

  void loop()
  {
    /* Here, I'm using the function random(range) to send a random value to the 
     ThingSpeak API. You can change this value to any sensor data
     so that the API will show the sensor data  
    */
    Serial.print("BPM: ");

    Serial.println(heart_rate);
    
    temp = analogRead(tempPin);
   // read analog volt from sensor and save to variable temp
    temp = temp * 0.48828125;
   // convert the analog volt to its temperature equivalent
    Serial.print("TEMPERATURE = ");
    Serial.print(temp); // display temperature value
    Serial.print("*C");
    Serial.println();
    sendVal = temp; // Send a random number between 1 and 1000

    sendVal2= heart_rate;
    
    String sendData = "GET /update?api_key="+ myAPI +"&"+ myFIELD +"="+String(sendVal);
    espData("AT+CIPMUX=1", 1000, DEBUG);       //Allow multiple connections
    espData("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG);
    espData("AT+CIPSEND=0," +String(sendData.length()+4),1000,DEBUG);  
    espSerial.find(">"); 
    espSerial.println(sendData);
    Serial.print("Value to be sent: ");
    Serial.println(sendVal);

    String sendData2 = "GET /update?api_key="+ myAPI +"&"+ myFIELD2 +"="+String(sendVal2);
    espData("AT+CIPMUX=1", 1000, DEBUG);       //Allow multiple connections
    espData("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG);
    espData("AT+CIPSEND=0," +String(sendData2.length()+4),1000,DEBUG);  
    espSerial.find(">"); 
    espSerial.println(sendData2);
    Serial.print("Value to be sent: ");
    Serial.println(sendVal2);
     
    espData("AT+CIPCLOSE=0",1000,DEBUG);
    delay(5000);
  }

  String espData(String command, const int timeout, boolean debug)
{
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");
  
  String response = "";
  espSerial.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (espSerial.available())
    {
      char c = espSerial.read();
      response += c;
    }
  }
  if (debug)
  {
    //Serial.print(response);
  }
  return response;
}

void interruptSetup()

{    

  TCCR2A = 0x02;  // This will disable the PWM on pin 3 and 11

  OCR2A = 0X7C;   // This will set the top of count to 124 for the 500Hz sample rate

  TCCR2B = 0x06;  // DON'T FORCE COMPARE, 256 PRESCALER

  TIMSK2 = 0x02;  // This will enable interrupt on match between OCR2A and Timer

  sei();          // This will make sure that the global interrupts are enable

}


ISR(TIMER2_COMPA_vect)

{ 

  cli();                                     

  analog_data = analogRead(sensor_pin);            

  samplecounter += 2;                        

  int N = samplecounter - lastBeatTime;      

  if(analog_data < thresh && N > (time_between_beats/5)*3)

    {     

      if (analog_data < trough_value)

      {                       
        trough_value = analog_data;
      }

    }


  if(analog_data > thresh && analog_data > peak_value)

    {        
      peak_value = analog_data;
    }                          



   if (N > 250)

  {                            

    if ( (analog_data > thresh) && (pulse_signal == false) && (N > (time_between_beats/5)*3) )

      {       

        pulse_signal = true;          

        time_between_beats = samplecounter - lastBeatTime;

        lastBeatTime = samplecounter;     

       if(second_heartpulse)

        {                        

          second_heartpulse = false;   

          for(int i=0; i<=9; i++)    

          {            

            beat[i] = time_between_beats; //Filling the array with the heart beat values                    

          }

        }


        if(first_heartpulse)

        {                        

          first_heartpulse = false;

          second_heartpulse = true;

          sei();            

          return;           

        }  

      word runningTotal = 0;  

      for(int i=0; i<=8; i++)

        {               

          beat[i] = beat[i+1];

          runningTotal += beat[i];

        }

      beat[9] = time_between_beats;             

      runningTotal += beat[9];   

      runningTotal /= 10;        

      heart_rate = 60000/runningTotal;

    }                      

  }

  if (analog_data < thresh && pulse_signal == true)

    {  

      pulse_signal = false;             

      amplitude = peak_value - trough_value;

      thresh = amplitude/2 + trough_value; 

      peak_value = thresh;           

      trough_value = thresh;

    }


  if (N > 2500)

    {                          

      thresh = 512;                     

      peak_value = 512;                 

      trough_value = 512;               

      lastBeatTime = samplecounter;     

      first_heartpulse = true;                 

      second_heartpulse = false;               

    }

  sei();                                
  
}
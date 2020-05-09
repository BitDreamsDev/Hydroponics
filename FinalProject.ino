#define DEBUG false
#include <SoftwareSerial.h>
String AP_SSID = "Whenever You Think";
String AP_PASSWORD = "30443044";
int RST_WAIT_TIME = 5000;
int WAIT_TIME = 5000;
boolean is_wifi_setup = false;
SoftwareSerial wifiComm(2,3); // RX, TX


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  wifiComm.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Starting\r\n");
}

void loop() {
  if(!is_wifi_setup){
    delay(2000);
    blink_n(10);
    delay(2000);
    if(sendData("AT+RST\r\n", RST_WAIT_TIME, DEBUG)){
      blink_n(4);
    }
    if(sendData("AT+CWMODE=1\r\n",WAIT_TIME, DEBUG)){
      blink_n(6);
    }
    if(sendData("AT+CWJAP=\"" + AP_SSID + "\",\"" + AP_PASSWORD + "\"\r\n", WAIT_TIME, DEBUG)){
      blink_n(8);
    }
    if(sendData("AT+CIPMUX=1\r\n", WAIT_TIME, DEBUG)){
      blink_n(10);
    }
    if(sendData("AT+CIPSERVER=1,80\r\n", WAIT_TIME, DEBUG)){
      blink_n(12);
    }
    is_wifi_setup = true;
  }
  
  // put your main code here, to run repeatedly:
//  if(Serial.available()){
//    delay(1000);
//    Serial.read(); // Clear buffer cause we don't care about the connect IDs
//    sendData("AT+CIPSEND=0,21\r\n", 2000, DEBUG);
//    sendData("<h1>Hello World!</h1>\r\n", 2000, DEBUG);
//    sendData("AT+CIPCLOSE=0\r\n", 2000, DEBUG);
//  }
}


boolean sendData(String command, const int timeout, boolean debug) {
  Serial.print("Attempting to send " + command + "\r\n");
  blink_n(2);
  boolean was_serial_available = false;
  String response = "";
  wifiComm.print(command);
  wifiComm.listen();
  long int time = millis();
  while( (time+timeout) > millis()){
    while(wifiComm.available() > 0){
      was_serial_available = true;
      Serial.print("We hit the codeblock!\r\n");
      // The esp has data so display its output to the serial window
      char c = wifiComm.read(); // read the next character.
      response+=c;
    }
  }
  
  if(debug)
  {
  Serial.print(response);
  }
  return was_serial_available;
}

// the loop function runs over and over again forever
void blink_n(int number_of_blinks) {
  for(int i=0;i<number_of_blinks;i++){
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(100);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(100);                       // wait for a second
  }
}

void blink_error(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
}

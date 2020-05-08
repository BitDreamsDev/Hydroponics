#define DEBUG true
String AP_SSID = "ChangeThisToYourSSID";
String AP_PASSWORD = "ChangeThisToYourPassword";


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  sendData("AT+RST\r\n", 2000, DEBUG);
  sendData("AT+CWMODE=1\r\n",2000, DEBUG);
  sendData("AT+CWJAP=\"" + AP_SSID + "\",\"" + AP_PASSWORD + "\"\r\n", 2000, DEBUG);
  sendData("AT+CIPMUX=1\r\n", 2000, DEBUG);
  sendData("AT+CIPSERVER=1,80\r\n", 2000, DEBUG);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    sendData("AT+CIPSEND=0,21\r\n", 2000, DEBUG); // Requires 0, bytes to send (Sample: hello world)
    sendData("<h1>Hello world!</h1>\r\n", 2000, DEBUG); // Sample text
    sendData("AT+CIPCLOSE=0\r\n", 2000, DEBUG); // Close to finish sending
  }
}

String sendData(String command, const int timeout, boolean debug) {
  String response = "";
  Serial.print(command);
  long int time = millis();
  while( (time+timeout) > millis()){
    while(Serial.available()){
      // The esp has data so display its output to the serial window
      char c = Serial.read(); // read the next character.
      response+=c;
      }
  }
  if(debug) {
  Serial.print(response);
  }
  return response;
}

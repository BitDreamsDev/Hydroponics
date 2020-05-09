// timing
#define SUCCESS_RATE 2000
#define ERROR_RATE 200
#define LOOP_RATE 1000
#define BAUD_RATE 9600

// line ending
#define CRLF "\r\n"

// error codes
#define SUCCESS 1
#define E_RST 2
#define E_CWMODE 3
#define E_CIFSR 4
#define E_CIPMUX 5
#define E_CIPSERVER 6

void blink(int nrBlinks, int rate);
bool sendCommand(char *command, char *ack);

// global state
bool g_failed = false;
int g_errnum = 0;

// fail fast; when there is a problem we'll loop the error code
// so the user can restart the arduino
void fail(int errnum) {
    g_errnum = errnum;
    g_failed = true;
}

void setup()
{  
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(BAUD_RATE);
    delay(1000);
    
    // reset module
    if (!sendCommand("AT+RST", "Ready")) {
        fail(E_RST);
        return;                    
    }
    
    // configure as access point
    if (!sendCommand("AT+CWMODE=2", "OK")) {
        fail(E_CWMODE);  
        return;
    }

    // get ip address
    if (!sendCommand("AT+CIFSR", "OK")) {
        fail(E_CIFSR);
        return;                
    }

    // configure for multiple connections
    if (!sendCommand("AT+CIPMUX=1", "OK")) {
        fail(E_CIPMUX);
        return;          
    }

    // turn on server on port 80
    if (!sendCommand("AT+CIPSERVER=1,80", "OK")) {
        fail(E_CIPSERVER);
        return;
    }

    // setup is complete, blink once to indicate success
    blink(SUCCESS, SUCCESS_RATE);
}

void loop()
{
    // setup failed, blink the error code until the user restarts
    // the board
    if (g_failed) {
        blink(g_errnum, ERROR_RATE);
    } else {
        blink(SUCCESS, SUCCESS_RATE);
    }
    
    delay(LOOP_RATE);
}

/*
* Sends a command over serial, and waits for a response. This
* function is synchronous.
* 
* command - the data/command to send; 
* 
* ack     - the expected response; must be of non-zero length
* 
* Returns true if the expected response was received
*/
bool sendCommand(const char *command, const char *ack)
{
    // send the command
    Serial.print(command);
    Serial.print(CRLF);

    // match the acknowledgement:

    // scan the stream for the beginning of the response
    for(;;) {
        // wait for data
        while (!Serial.available());

        // discard characters until we've found the first in the ack
        if (ack[0] == Serial.read()) {
            break;
        }
    }
    
    // ensure the rest of the token is present after that
    int i = 1;
    for (; i < strlen(ack) - 1; i++) {
        // match each character to the next in the stream
        while(!Serial.available());
        char ch = Serial.read();

        // if at any point the response doesn't match, fail
        if (ack[i] != ch) {
            return false;
        }    
    }

    // We'll be permissive and not check the CRLF here, since it's a PITA
    // to send a CRLF with PuTTY. We will assume anything after the correct
    // response is a CRLF. It will be discarded on the next sendCommand()
    // when we seek to the begginning of the ack.
    
    // if we reach this point, we have matched the ack
    return true;
}

/** Blinks the built-in led `nrBlinks' times at `rate' */
void blink(int nrBlinks, int rate) 
{
    // turn the LED on and off
    for(int i = 0; i < nrBlinks; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(rate);
        digitalWrite(LED_BUILTIN, LOW); 
        delay(rate);
    }
}

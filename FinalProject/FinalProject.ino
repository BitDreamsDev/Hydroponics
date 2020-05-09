#include <stdio.h>

// line ending
#define CRLF "\r\n"

// timing
#define TIMEOUT 5000
#define SUCCESS_RATE 500
#define ERROR_RATE 200
#define LOOP_RATE 1000

// you should change the baud rate of the ESP 8266, use the following
// AT command:
//            AT+UART_DEF=9600,8,1,0,0
//
// this change is persistent
#define BAUD_RATE 9600

// error codes
#define SUCCESS 1

#define E_RST 2
#define E_CWMODE 3
#define E_CIFSR 4
#define E_CIPMUX 5
#define E_CIPSERVER 6
#define E_CWJAP 7
#define E_CIPCLOSE 8
#define E_CIPSEND 9

// configuration
#define AP_SSID "Meeseeks"
#define AP_PASSWORD "3X1573NC315P41N"

// prototypes
void blink(int nrBlinks, int rate);
bool sendCommand(char *command, char *ack);
void fail(int errnum);
bool waitForTimeout();

// global state
bool g_failed = false;
int g_errnum = 0;

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

    // configure as client
    if (!sendCommand("AT+CWMODE=1", "OK")) {
        fail(E_CWMODE);
        return;
    }

    // authenticate
    char cmd[64] = {0};
    snprintf(cmd, sizeof cmd, "AT+CWJAP=\"%s\",\"%s\"", AP_SSID, AP_PASSWORD);
    if (!sendCommand((const char *) cmd, "OK")) {
        fail(E_CWJAP);
        return;
    }

    // get ip address
    //    if (!sendCommand("AT+CIFSR", "OK")) {
    //    fail(E_CIFSR);
    //    return;
    //}

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
        return;
    }

    // Send web page info
    if (!sendCommand("AT+CIPSEND=0,21", "OK")) {
        fail(E_CIPSEND);
        return;
    }

    Serial.println("<h1>Hello World!</h1>");

    if (!sendCommand("AT+CIPCLOSE=0", "OK")) {
        fail(E_CIPCLOSE);
        return;
    }
}

/**
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

    // scan the stream for the beginning of the response, this also
    // needs its own timeout to prevent denial of service
    unsigned long t1 = millis();
    for(;;) {
        // wait for data
        if (!waitForTimeout()) return false;

        // discard characters until we've found the first in the ack
        if (ack[0] == Serial.read()) {
            break;
        }

        // check if the specific timeout was exceeded (we should timeout not
        // only waiting for data here, but waiting for the right data)
        if (t1 + TIMEOUT < millis()) {
            return false;
        }
    }

    // ensure the rest of the token is present after that
    int i = 1;
    for (; i < strlen(ack) - 1; i++) {
        // match each character to the next in the stream
        if (!waitForTimeout()) return false;
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

/** fail fast; when there is a problem we'll loop the error code
    so the user can restart the arduino */
void fail(int errnum) {
    g_errnum = errnum;
    g_failed = true;
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

/** polls serial bus for data until TIMEOUT, returns false on failure
    to acquire data */
bool waitForTimeout() {
    unsigned long t1 = millis();
    bool timedOut = false;
    while (!Serial.available() && !timedOut) {
        timedOut = t1 + TIMEOUT < millis();
    }
    return !timedOut;
}

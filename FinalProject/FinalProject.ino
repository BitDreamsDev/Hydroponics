/*
Copyright (c) 2020 BitDreams, LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>

// line ending
#define CRLF "\r\n"

// limits
#define PAGE_MAX 256
#define CMD_MAX 64

// pins
#define RELAY_PIN 9

// measurement
#define WATER_LOW 300
#define WATER_MID 323
#define WATER_HIGH 345

// timing
#define REQUEST_DELAY 100
#define TIMEOUT 10000
#define SUCCESS_RATE 500
#define ERROR_RATE 200
#define LOOP_RATE 1000

// this change is persistent
#define BAUD_RATE 115200

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
#define E_ACCEPT 10
#define E_FATAL 11

// configuration
#define AP_SSID "Meeseeks"
#define AP_PASSWORD "3X1573NC315P41N"

// prototypes
void blink(int nrBlinks, int rate);
bool sendCommand(char *command, char *ack);
void fail(int errnum);
int readNext();
bool findToken(const char *token);
const char *descWaterLevel();

// global state
bool g_failed = false;
int g_errnum = 0;
bool g_pump_state = false;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(BAUD_RATE);
    delay(2000);

    // setup relay pin
    pinMode(RELAY_PIN, OUTPUT);

    // reset module
    if (!sendCommand("AT+RST", "OK")) {
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
    if (!sendCommand((const char *) cmd, "WIFI CONNECTED")) {
        fail(E_CWJAP);
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
    delay(LOOP_RATE);

    // setup failed, blink the error code until the user restarts
    // the board
    if (g_failed) {
        blink(g_errnum, ERROR_RATE);
        return;
    }

    // handle TCP connections as they arrive
    if(findToken("+IPD,"))
    {
        // read connection id
        int ch = -1;
        int connId = -1;
        if ((ch = readNext()) != -1) {
            connId = ch - '0';
        } else {
            fail(E_ACCEPT);
            return;
        }

        blink(1, ERROR_RATE);

        // handle HTTP requests
        if (findToken("GET /")) {

            // check if the user is changing the relay state
            if (findToken("?q=", REQUEST_DELAY) && (ch = readNext()) != -1) {
                if (ch == '0') {
                    g_pump_state = false;
                    digitalWrite(RELAY_PIN, LOW);
                } else if (ch == '1') {
                    g_pump_state = true;
                    digitalWrite(RELAY_PIN, HIGH);
                }
            }

            int waterValue = analogRead(A0);
            const char *waterLevel = descWaterLevel(waterValue);

            // generate web page
            char page[PAGE_MAX] = {0};
            snprintf(page, sizeof page, "<h1>Water Level: %s, Value: %d</h1>\n"
                                        "<h1>Pump State %s</h1>\n"
                                        "<a href=\"http://192.168.1.195/?q=0\">Pump Off</a>"
                                        "<br />"
                                        "<a href=\"http://192.168.1.195/?q=1\">Pump On</a>",
                     waterLevel, waterValue, g_pump_state ? "On" : "Off");

            // send CIPSEND command
            char cmd[CMD_MAX] = {0};
            snprintf(cmd, sizeof cmd, "AT+CIPSEND=%d,%d", connId, strlen(page));
            if (!sendCommand((const char *) cmd, "OK")) {
                blink(3, ERROR_RATE);
                return;
            }

            // send webpage
            delay(REQUEST_DELAY);
            if (!sendCommand((const char *) page, "SEND OK")) {
                blink(4, ERROR_RATE);
                return;
            }

            // close HTTP connection
            memset(cmd, 0, sizeof cmd);
            snprintf(cmd, sizeof cmd, "AT+CIPCLOSE=%d", connId);

            if (!sendCommand((const char *)cmd, "OK")) {
                blink(5, ERROR_RATE);
                return;
            }
        }
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
    return findToken(ack);
}


bool findToken(const char *token, long timeout) {
    // scan the stream for the beginning of the response, this also
    // needs its own timeout to prevent denial of service
    unsigned long t1 = millis();

    while(1) {
        // try to match the token
        int ch, i = 0;
        while (i < strlen(token) && (ch = readNext(timeout)) != -1) {
            if (ch == token[i]) {
                i++;
            } else {
                i = 0;
            }
        }

        if (ch == -1) {
            return false;
        }

        if (i == strlen(token)) {
            return true;
        }
    }
}

/** seeks to the next occurrence of token, or times out */
bool findToken(const char *token) {
    return findToken(token, TIMEOUT);
}

/** fail fast; when there is a problem we'll loop the error code
    so the user can restart the arduino */
void fail(int errnum) {
    g_errnum = errnum;
    g_failed = true;
}

/** returns a pointer to a string description of the water level */
const char *descWaterLevel(int sensorVal) {

    if(sensorVal <= WATER_LOW) {
        return "empty";
    }
    else if(sensorVal > WATER_LOW && sensorVal <= WATER_MID) {
        return "low";
    }
    else if(sensorVal > WATER_MID && sensorVal <= WATER_HIGH) {
        return "medium";
    }
    else {
        return "full";
    }
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

/** reads the next character, unless it times out, then returns -1 */
int readNext() {
  return readNext(TIMEOUT);
}

int readNext(unsigned long timeout) {
    unsigned long t1 = millis();
    bool timedOut = false;

    while (!Serial.available() && !timedOut) {
        timedOut = t1 + timeout < millis();
    }

    // we don't need to check the timeout here because Serial.read() will
    // return -1 if there isn't anything buffered
    return Serial.read();
}

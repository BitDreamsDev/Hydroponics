#include <stdio.h>

// line ending
#define CRLF "\r\n"

// limits
#define PAGE_MAX 64
#define CMD_MAX 64

// timing
#define REQUEST_DELAY 20
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

// configuration
#define AP_SSID "Meeseeks"
#define AP_PASSWORD "3X1573NC315P41N"

// prototypes
void blink(int nrBlinks, int rate);
bool sendCommand(char *command, char *ack);
void fail(int errnum);
int readNext();
bool findToken(const char *token);

// global state
bool g_failed = false;
int g_errnum = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(BAUD_RATE);
    delay(2000);

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
            // generate web page
            char page[PAGE_MAX] = {0};
            snprintf(page, sizeof page, "<h1>Hello, World!</h1>%s", CRLF);

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

/** seeks to the next occurrence of token, or times out */
bool findToken(const char *token) {
    // scan the stream for the beginning of the response, this also
    // needs its own timeout to prevent denial of service
    unsigned long t1 = millis();

    while(1) {
        // try to match the token
        int ch, i = 0;
        while (i < strlen(token) && (ch = readNext()) != -1) {
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

/** reads the next character, unless it times out, then returns -1 */
int readNext() {
    unsigned long t1 = millis();
    bool timedOut = false;

    while (!Serial.available() && !timedOut) {
        timedOut = t1 + TIMEOUT < millis();
    }

    // we don't need to check the timeout here because Serial.read() will
    // return -1 if there isn't anything buffered
    return Serial.read();
}

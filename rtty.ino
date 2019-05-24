/* *****************************************************************************
 *  RTTY Routines - Transmit/Receive BAUDOT using ARDUINO digital pins
 *  Receive routine relies in 1 millisecond timer (FlexiTimer2)
*  *****************************************************************************/

#define LETTERS 31
#define FIGURES 27
#define BCR 8
#define BLF 2
#define BSPACE 4
#define RTTYBUFFSIZE 64
#define RTTYRXPIN 14     // RTTY Data OUT - 45.45 baud BitBang
#define RTTYTXPIN 15     // RTTY Data IN
#define RTTYRTSPIN 16    // RTTY RTS pin

// BAUDOT to ASCII lookup tables
char ltrs[]{'\0','E','\n','A',' ','S','I','U','\r','D','R','J','N','F','C','K',
            'T','Z','L','W','H','Y','P','Q','O','B','G','<','M','X','V','>'};
char figs[]{'\0','3','\n','-',' ','\a','8','7','\r','$','4','\'',',','!',':','(',
            '5','"',')','2','#','6','0','1','9','?','&','<','.','/',';','>'};
            
// ASCII to BAUDOT lookup table - automatic lower to upper case conversion
// CTL-F = Figures and CTL-L = Letters
int8_t baudot[] {0, 0, 0, 0, 0, 0, 27, 5, 0, 0, 2, 0, 31, 8, 0, 0,
                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 4, 13, 17, 20, 9, 0, 26, 11, 15, 18, 0, 0, 12, 3, 28, 29,
                 22, 23, 19, 1, 10, 16, 21, 7, 6, 24, 14, 30, 0, 0, 0, 25, 0,
                 3, 25, 14, 9, 1, 13, 26, 20, 6, 11, 15, 18, 28, 12, 24,
                 22, 23, 10, 5, 16, 7, 30, 19, 29, 21, 17, 0, 0, 0, 0, 0, 0,
                 3, 25, 14, 9, 1, 13, 26, 20, 6, 11, 15, 18, 28, 12, 24,
                 22, 23, 10, 5, 16, 7, 30, 19, 29, 21, 17, 0, 0, 0, 0, 0};
                 
char rtty_buff[RTTYBUFFSIZE];
uint8_t rtty_buffhead = 0, rtty_bufftail = 0;
uint8_t rtty_bittime;
bool rtty_is_sending = false;
char rtty_tdr;           // Transmit Data Register
bool rtty_tdre = true;   // Transmit Data Register Empty Flag

// Set up digital pins
void rttyInit(){
    pinAsOutput(RTTYTXPIN);
    pinAsInput(RTTYRXPIN);
    pinAsOutput(RTTYRTSPIN);
    digitalHigh(RTTYRTSPIN);  // Set RTS off
    digitalHigh(RTTYTXPIN);   // Set MARK idle
}

// Menu function to set RTTY bit time
void setRttyBittime(uint8_t ms){
  rtty_bittime = ms;
}

// Read data from rtty buffer - return ASCII char
char rttyRead()
{
    static bool _figs;
    char ch, baudotchar;
    
    if (rtty_buffhead == rtty_bufftail)   // Should not need this if we use rttyBuffAvailable()
        return -1;
    baudotchar = rtty_buff[rtty_buffhead];
    rtty_buffhead = (rtty_buffhead + 1) % RTTYBUFFSIZE;
    
    if(baudotchar == LETTERS)
        _figs = false;
    if(uos == true && baudotchar == BSPACE)
        _figs = false;
    else if(baudotchar == FIGURES)
        _figs = true;
    if(_figs == false)
        ch = ltrs[baudotchar];
    else
        ch = figs[baudotchar];
    if(debug == false && (ch == '<' || ch == '>')) ch = '\0';
    return ch;
}

void rttyBufferPut(uint8_t ch){
    if ((rtty_bufftail + 1) % RTTYBUFFSIZE != rtty_buffhead){
        rtty_buff[rtty_bufftail] = ch; // save new byte
        rtty_bufftail = (rtty_bufftail + 1) % RTTYBUFFSIZE;
    }
}

// Return number of characters in rtty buffer
uint8_t rttyBuffAvailable(){
    return (rtty_bufftail + RTTYBUFFSIZE - rtty_buffhead) % RTTYBUFFSIZE;
}

// RTTY Bit-Bang Serial. 1 Start bit, 5 Data bits, 1.5 Stop bits.
// Baud rate determined by rtty_bittime (milliseconds). TX pin is RTTYTXPIN.
// Uses delay() vs delayMicroseconds() because of inaccuracy above a count of 16383
void rttySendBaudot(char c)
{
    rtty_is_sending = true;
    rttyResetRTS();
    // Start bit
    digitalLow(RTTYTXPIN);
    delay(rtty_bittime);
    // Data bits
    for (int i = 0; i < 5; i++) {
        if(bitRead(c, i) == 1)
            digitalHigh(RTTYTXPIN);
        else
            digitalLow(RTTYTXPIN);
        delay(rtty_bittime);
    }
    // 1.5 stop bits
    digitalHigh(RTTYTXPIN);
    delay(rtty_bittime * 1.5);
    rtty_is_sending = false;
}

char ttySendBaudotX(char c)
{
    rtty_tdr = c;
    rtty_tdre = false;
}

// Send character converted to BAUDOT
void rttySendChar(char ch)
{
    static bool _figs;
    if (ch > 64 && ch < 128) {              // LETTERS are 65 and above
        if (_figs == false) rttySendBaudot(baudot[ch]);
        else {
            rttySendBaudot(LETTERS);
            _figs = false;
            rttySendBaudot(baudot[ch]);
        }
    }
    else if (ch < 65 && ch > 0) {           // FIGURES are less than 65
        // CR, LF, & SPACE in both FIGS & LTRS. CTL-F = FIGURES, CTL-L = LETTERS
        if (ch == 10 || ch == 13 || ch == 32 || ch == 6 || ch == 12) rttySendBaudot(baudot[ch]);
        else if (_figs == true) rttySendBaudot(baudot[ch]);
        else {
            rttySendBaudot(FIGURES);
            _figs = true;
            rttySendBaudot(baudot[ch]);
        }
        // If we just sent a linefeed, follow it with LTRS to get in a known state at the beggining of a line ala ITTY
        if (ch == 13) rttySendBaudot(LETTERS);
        // Set figs/ltrs if needed
        if (ch == 6) _figs = true;
        if (ch == 12) _figs = false;
    }
    // ttySendChar(ch);   // test
}

// Send string converted to BAUDOT
void rttySendString(String msg) {
    rttySendBaudot(LETTERS);             // start in a known state
    for (int i = 0; i < msg.length(); i++) {
        rttySendChar(msg.charAt(i));
    }
}

// RTTY Software UART Receiver - 1 start bit, 5 data bits, 1.5 stop bits
// Character is saved in Global rtty_buff[].
void rttyRxHandler(void)
{
    static uint8_t UART_State;
    static uint8_t baudot;
    static uint8_t sample_time;

    sample_time++;
    // RTTY MARK is idle condition.
    // RTTY SPACE signifies start of data.
    if(UART_State == WAITING && digitalState(RTTYRXPIN) == SPACE && digitalRead(RTTYRTSPIN) == HIGH){
        UART_State = INSTARTBIT;
        sample_time = 0;
    }
    // If we still have a SPACE after 1/2 rtty_bittime (+ 0-1 ms), call it a START bit.
    // (Since we use a 1 ms timer, we have a 1 ms uncertainty.)
    // Sample will occur 1/2 rtty_bittime + 0-1 ms after leading edge of start bit is detected.
    else if(UART_State == INSTARTBIT && sample_time == rtty_bittime / 2){
        if(digitalState(RTTYRXPIN) == SPACE){
           UART_State = INDATA;
           sample_time = 0;
        }
        else
           UART_State = WAITING;
    }
    // Sample data at center of each bit every rtty_bittime milliseconds
    else if(UART_State == INDATA){
        if(sample_time == rtty_bittime)
            bitWrite(baudot, 0, digitalState(RTTYRXPIN));
        else if(sample_time == rtty_bittime * 2)
            bitWrite(baudot, 1, digitalState(RTTYRXPIN));
        else if(sample_time == rtty_bittime * 3)
            bitWrite(baudot, 2, digitalState(RTTYRXPIN));
        else if(sample_time == rtty_bittime * 4)
            bitWrite(baudot, 3, digitalState(RTTYRXPIN));
        else if(sample_time == rtty_bittime * 5)
            bitWrite(baudot, 4, digitalState(RTTYRXPIN));
        else if(sample_time == rtty_bittime * 6)
        {
            // If we have a MARK now, it's a STOP bit. Put character in buffer.
            if(digitalState(RTTYRXPIN) == MARK){
                rttyBufferPut(baudot);
            }
            UART_State = WAITING;
        }
    }
}

// RTTY Software UART Transmitter ***EXPERIMENTAL***
void rttyTxHandler(void)
{
    static uint8_t UART_State;
    static uint8_t sample_time;
    
    sample_time++;
    if(rtty_tdre == true) 
        UART_State = WAITING;
    if (UART_State == WAITING && rtty_tdre == false) {
        UART_State = INDATA;
        sample_time = 0;
    }
    else if (UART_State == INDATA) {
        if (sample_time == 1)         // Start Bit
            digitalWrite(RTTYTXPIN, LOW);
        else if (sample_time == rtty_bittime + 1)
            digitalWrite(RTTYTXPIN, bitRead(rtty_tdr, 0));    // Bit 0
        else if (sample_time == (2 * rtty_bittime) + 1)
            digitalWrite(RTTYTXPIN, bitRead(rtty_tdr, 1));    // Bit 1
        else if (sample_time == (3 * rtty_bittime) + 1)
            digitalWrite(RTTYTXPIN, bitRead(rtty_tdr, 2));    // Bit 2
        else if (sample_time == (4 * rtty_bittime) + 1)
            digitalWrite(RTTYTXPIN, bitRead(rtty_tdr, 3));    // Bit 3
        else if (sample_time == (5 * rtty_bittime) + 1)
            digitalWrite(RTTYTXPIN, bitRead(rtty_tdr, 4));    // Bit 4
        else if (sample_time == (6 * rtty_bittime) + 1)
            digitalWrite(RTTYTXPIN, HIGH);   // Stop Bits
        else if (sample_time == (7.5 * rtty_bittime) + 1){
            rtty_tdre = true;               // All bits transmitted
        }
    }     
}

void rttyResetRTS(void){
    rts_timeout = 1000;
    digitalLow(RTTYRTSPIN); // RTS ON
}

void rttyRTSTimer(void){
    rts_timeout--;
    if(rts_timeout <=0) rts_timeout = 0;
    if(rts_timeout == 0) digitalHigh(RTTYRTSPIN); // RTS OFF
}

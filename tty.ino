
/* *****************************************************************************
 *  TTY Routines - Transmit/Receive ASCII 110 baud using ARDUINO digital pins
 *  Primarily for Model 33 Teletype - 110 baud 8N2
 *  Receive routine relies in 1 millisecond timer (FlexiTimer2)
*  *****************************************************************************/
#define TTYBUFFSIZE 64
#define TTYBITTIME 9    // 110 baud (9 ms per bit) == uses delay() (real value is 9.09 ms but 9 is OK)
#define TTYTXPIN 5      // TTY Data OUT - 110 baud BitBang
#define TTYRXPIN 6      // TTY Data IN
#define TTYRDRPIN 7     // TTY Reader Control
#define RTSDELAYTIME 100   // 2 second RTS delay

char tty_buff[TTYBUFFSIZE];
uint8_t tty_buffhead = 0, tty_bufftail = 0;
uint8_t x_state = XON;
char tdr;           // Transmit Data Register
bool tdre = true;   // Transmit Data Register Empty Flag

// Set up digital pins
void ttyInit(){
    pinAsOutput(TTYTXPIN);
    pinAsInput(TTYRXPIN);
    pinAsOutput(TTYRDRPIN);
    digitalHigh(TTYTXPIN);  // Set MARK idle
    digitalHigh(TTYRDRPIN); // Enable Reader
}

// Read data from tty buffer
char ttyRead()
{
    if (tty_buffhead == tty_bufftail)
        return -1;
    char achar = tty_buff[tty_buffhead];
    tty_buffhead = (tty_buffhead + 1) % TTYBUFFSIZE;
    // Control buffer fill if using tape reader - CTS Reader Control protocol
    if(reader_control == RDR_BUFFER){
        if(ttyBuffAvailable() > 0.9 * TTYBUFFSIZE ){
          digitalLow(TTYRDRPIN);   // Buffer alomst full, Turn off reader
        }
        else if(ttyBuffAvailable() < 0.1 * TTYBUFFSIZE ){
          digitalHigh(TTYRDRPIN);    // Buffer almost empty, Turn on reader
        }
    }
    return achar;
}

// Return number of characters in tty buffer
uint8_t ttyBuffAvailable(){
    return (tty_bufftail + TTYBUFFSIZE - tty_buffhead) % TTYBUFFSIZE;
}

// TTY Bit-Bang Serial. 1 Start bit, 8 Data bits, 2 Stop bits.
// Baud rate determined by TTYBITTIME (milliseconds). TX pin is TTYTXPIN.
// To be 100% correct, we should delay 9.09 ms., but 9 ms is only 1% off.
// UARTs can handle 3-4% easily. The Model 33 handles it fine.
char ttySendChar(char c)
{
    tty_isSending = true;
    // Start bit
    digitalLow(TTYTXPIN);
    delay(TTYBITTIME);
    // Data bits
    for (uint8_t i=0; i < 8; i++){
        digitalWrite(TTYTXPIN, bitRead(c, i));
        delay(TTYBITTIME);
    }
    // Two stop bits
    digitalHigh(TTYTXPIN);
    delay(TTYBITTIME * 2);
    tty_isSending = false;
    return c;
}

char ttySendCharX(char c)
{
    tdr = c;
    tdre = false;
}

// TTY Software UART Receiver - 110 baud, 1 start bit, 8 data bits, 2 stop bits
// Sample times hard-coded for speed optimization
// Character is saved in Global tty_buff[].
void ttyRxHandler(void)
{
    static uint8_t UART_State;
    static uint8_t databyte;
    static uint8_t sample_time;
    static bool receiving = false;
    
    sample_time++;

    if (UART_State == WAITING && digitalState(TTYRXPIN) == SPACE) {
        UART_State = INSTARTBIT;
        sample_time = 0;
        receiving = true;
    }
    else if (UART_State == INSTARTBIT && sample_time == 4) {
        if (digitalState(TTYRXPIN) == SPACE) {
            UART_State = INDATA;
            sample_time = 0;
        }
        else
            UART_State = WAITING;
    }
    // Sample data at center of each bit every 9 ms.
    // This should be close enough to 9.09 ms. for 110 baud 8N2 TTY data.
    else if (UART_State == INDATA) {
        if (sample_time == 9)
            bitWrite(databyte, 0, digitalState(TTYRXPIN));
        else if (sample_time == 18)
            bitWrite(databyte, 1, digitalState(TTYRXPIN));
        else if (sample_time == 27)
            bitWrite(databyte, 2, digitalState(TTYRXPIN));
        else if (sample_time == 36)
            bitWrite(databyte, 3, digitalState(TTYRXPIN));
        else if (sample_time == 45)
            bitWrite(databyte, 4, digitalState(TTYRXPIN));
        else if (sample_time == 54)
            bitWrite(databyte, 5, digitalState(TTYRXPIN));
        else if (sample_time == 63)
            bitWrite(databyte, 6, digitalState(TTYRXPIN));
        else if (sample_time == 72)
            bitWrite(databyte, 7, digitalState(TTYRXPIN));
        else if (sample_time == 81) {
            // If we have a MARK now, it's a STOP bit. We got a character.
            if (digitalState(TTYRXPIN) == MARK) {
                databyte = (char)(databyte); // drop high bit (MARK Parity on TTY)
                if ((tty_bufftail + 1) % TTYBUFFSIZE != tty_buffhead){
                    tty_buff[tty_bufftail] = databyte; // save new byte
                    tty_bufftail = (tty_bufftail + 1) % TTYBUFFSIZE;
                }
            }
        UART_State = WAITING;
        }
    }

    // echo only while characters are actually being sent from keyboard or reader
    if(receiving == true){
        if(echo == true)
            digitalWrite(TTYTXPIN, digitalRead(TTYRXPIN));
        if(sample_time >= 90){
            receiving = false;
        }        
    }       
}

// TTY Software UART Transmitter - 110 baud, 1 start bit, 8 data bits, 2 stop bits
// Sample times hard-coded for speed optimization
// Need to use this for echoing macros in close to real time
void ttyTxHandler(void)
{
    static uint8_t UART_State;
    static uint8_t sample_time;
    
    sample_time++;
    if(tdre == true) 
        UART_State = WAITING;
    if (UART_State == WAITING && tdre == false) {
        UART_State = INDATA;
        sample_time = 0;
    }
    // This should be close enough to 9.09 ms. for 110 baud 8N2 TTY data.
    else if (UART_State == INDATA) {
        if (sample_time == 1)   // Start Bit
            digitalWrite(TTYTXPIN, LOW);
        else if (sample_time == 10)
            digitalWrite(TTYTXPIN, bitRead(tdr, 0));    // Bit 0
        else if (sample_time == 19)
            digitalWrite(TTYTXPIN, bitRead(tdr, 1));    // Bit 1
        else if (sample_time == 28)
            digitalWrite(TTYTXPIN, bitRead(tdr, 2));    // Bit 2
        else if (sample_time == 37)
            digitalWrite(TTYTXPIN, bitRead(tdr, 3));    // Bit 3
        else if (sample_time == 46)
            digitalWrite(TTYTXPIN, bitRead(tdr, 4));    // Bit 4
        else if (sample_time == 55)
            digitalWrite(TTYTXPIN, bitRead(tdr, 5));    // Bit 5
        else if (sample_time == 64)
            digitalWrite(TTYTXPIN, bitRead(tdr, 6));    // Bit 6
        else if (sample_time == 73)
            digitalWrite(TTYTXPIN, bitRead(tdr, 7));    // Bit 7
        else if (sample_time == 82)
            digitalWrite(TTYTXPIN, HIGH);   // Stop Bits
        else if (sample_time == 95){        // Changed to 95 (from 100) for echoing macros - seems to work OK
            tdre = true;                    // All bits transmitted
        }
    }     
}

// Handle CR and line wrap
void ttyFilter(char ch)
{
    static int charpos;

    if(crlf == true && ch != '\n' && ch != '\r'){
        if (ch > 0x1f) charpos = charpos + 1;
        HostSerial.print(ch);
        ttySendChar(ch);
    }
    
    if(crlf == false){
        if (ch > 0x1f) charpos = charpos + 1;
        HostSerial.print(ch);
        ttySendChar(ch);
        if(ch == '\r') charpos = 0;     
    }
        
    if((crlf == true && ch == '\n')){         // Handle LF add CR
        HostSerial.print('\r');
        ttySendChar('\r');      
        HostSerial.print('\n');
        ttySendChar('\n');
        charpos = 0;
    }
    
    if(linewrap > 0 && charpos == linewrap){  // Line wrap at 68 or 72 characters
        HostSerial.print('\r');
        ttySendChar('\r');
        HostSerial.print('\n');
        ttySendChar('\n');
        charpos = 0;
    }
}

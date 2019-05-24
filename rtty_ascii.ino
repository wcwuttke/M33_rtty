

/* *****************************************************************************
 *  ASCII RTTY Routines - Transmit/Receive ASCII 110 baud using ARDUINO digital pins
 *  Primarily for Model 33 Teletype - 110 baud 8N2
 *  Receive routine relies in 1 millisecond timer (FlexiTimer2)
*  *****************************************************************************/
#define ASCII_RTTY_BUFFSIZE 64
#define ASCII_RTTY_BITTIME 9     // 110 baud (9 ms per bit) == uses delay() (real value is 9.09 ms but 9 is OK)
#define ASCII_RTTY_RXPIN 14      // ASCII_RTTY_ Data IN
#define ASCII_RTTY_TXPIN 15      // ASCII_RTTY_ Data OUT - 110 baud BitBang
#define ASCII_RTTY_RTSPIN 16
#define ASCII_RTTY_CTSPIN 17     // ASCII_RTTY_ CTS for Reader Control
#define TTYRDRPIN 7

char ascii_rtty_buff[ASCII_RTTY_BUFFSIZE];
uint8_t ascii_rtty_buffhead = 0, ascii_rtty_bufftail = 0;

// Set up digital pins
void ascii_rttyInit(){
    pinAsOutput(ASCII_RTTY_TXPIN);
    pinAsInput(ASCII_RTTY_RXPIN);
    pinAsOutput(ASCII_RTTY_RTSPIN);
    pinMode(ASCII_RTTY_CTSPIN, INPUT_PULLUP);
    digitalHigh(ASCII_RTTY_TXPIN);  // Set MARK idle
    digitalHigh(ASCII_RTTY_RTSPIN);  // Set RTS off
}

// Read data from ascii_rtty buffer
char ascii_rttyRead()
{
    if (ascii_rtty_buffhead == ascii_rtty_bufftail)
        return -1;
    char achar = ascii_rtty_buff[ascii_rtty_buffhead];
    ascii_rtty_buffhead = (ascii_rtty_buffhead + 1) % ASCII_RTTY_BUFFSIZE;
    // Control buffer fill if using tape reader - CTS Reader Control protocol
//    if(ascii_rttyBuffAvailable() > 0.9 * ASCII_RTTY_BUFFSIZE ){
//      digitalLow(ASCII_RTTY_RDRPIN);   // Buffer alomst full, Turn off reader
//    }
//    else if(ascii_rttyBuffAvailable() < 0.1 * ASCII_RTTY_BUFFSIZE ){
//      digitalHigh(ASCII_RTTY_RDRPIN);    // Buffer almost empty, Turn on reader
//    }
    return achar;
}

// Return number of characters in ascii_rtty buffer
uint8_t ascii_rttyBuffAvailable(){
    return (ascii_rtty_bufftail + ASCII_RTTY_BUFFSIZE - ascii_rtty_buffhead) % ASCII_RTTY_BUFFSIZE;
}

// ASCII_RTTY_ Bit-Bang Serial. 1 Start bit, 8 Data bits, 2 Stop bits.
// Baud rate determined by ASCII_RTTY_BITTIME (milliseconds). TX pin is ASCII_RTTY_TXPIN.
// To be 100% correct, we should delay 9.09 ms., but 9 ms is only 1% off.
// UARTs can handle 3-4% easily. The Model 33 handles it fine.
char ascii_rttySendChar(char c)
{
    ascii_rtty_isSending = true;
    // Start bit
    digitalLow(ASCII_RTTY_TXPIN);
    delay(ASCII_RTTY_BITTIME);
    // Data bits
    for (uint8_t i=0; i < 8; i++){
        if(bitRead(c, i) == 1)
            digitalHigh(ASCII_RTTY_TXPIN);
        else
            digitalLow(ASCII_RTTY_TXPIN);
        delay(ASCII_RTTY_BITTIME);
    }
    // Two stop bits
    digitalHigh(ASCII_RTTY_TXPIN);
    delay(ASCII_RTTY_BITTIME * 2);
    ascii_rtty_isSending = false;
    return c;
}

// ASCII_RTTY_ Software UART Receiver - 110 baud, 1 start bit, 8 data bits, 2 stop bits
// Sample times hard-coded for speed optimization
// Character is saved in Global ascii_rtty_buff[].
void ascii_rttyRxHandler(void)
{
    static uint8_t UART_State;
    static uint8_t databyte;
    static uint8_t sample_time;

    sample_time++;

    if (UART_State == WAITING && digitalState(ASCII_RTTY_RXPIN) == SPACE) {
        UART_State = INSTARTBIT;
        sample_time = 0;
    }
    else if (UART_State == INSTARTBIT && sample_time == 4) {
        if (digitalState(ASCII_RTTY_RXPIN) == SPACE) {
            UART_State = INDATA;
            sample_time = 0;
        }
        else
            UART_State = WAITING;
    }
    // Sample data at center of each bit every 9 ms.
    // This should be close enough to 9.09 ms. for 110 baud 8N2 ASCII_RTTY_ data.
    else if (UART_State == INDATA) {
        if (sample_time == 9)
            bitWrite(databyte, 0, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 18)
            bitWrite(databyte, 1, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 27)
            bitWrite(databyte, 2, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 36)
            bitWrite(databyte, 3, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 45)
            bitWrite(databyte, 4, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 54)
            bitWrite(databyte, 5, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 63)
            bitWrite(databyte, 6, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 72)
            bitWrite(databyte, 7, digitalState(ASCII_RTTY_RXPIN));
        else if (sample_time == 81) {
            // If we have a MARK now, it's a STOP bit. We got a character.
            if (digitalState(ASCII_RTTY_RXPIN) == MARK) {
                databyte = (char)(databyte);
                if ((ascii_rtty_bufftail + 1) % ASCII_RTTY_BUFFSIZE != ascii_rtty_buffhead){
                    ascii_rtty_buff[ascii_rtty_bufftail] = databyte; // save new byte
                    ascii_rtty_bufftail = (ascii_rtty_bufftail + 1) % ASCII_RTTY_BUFFSIZE;
                }
            }
        UART_State = WAITING;
        }
    }
}

// New Reader Control via CTS from DCE
void ascii_rttyReaderControl(void)
{
    if(reader_control == RDR_CTS)
        digitalWrite(TTYRDRPIN, !digitalRead(ASCII_RTTY_CTSPIN));
    else if(reader_control == RDR_NONE)
        digitalWrite(TTYRDRPIN, HIGH);
}

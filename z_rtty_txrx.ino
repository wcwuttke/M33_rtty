/* *****************************************************************************
 *  Main Routine - Receive or Transmit RTTY using either a Model 33 Teletype
 *  and/or an RS-232 Terminal (9600 8N1) attached to the TX/RX pins and/or
 *  a software terminal program (9600 8N1) using the Arduino USB port serial connection.
*  *****************************************************************************/
#define SERBUFFSIZE 64

char ser_buff[SERBUFFSIZE];
uint8_t ser_buffhead = 0, ser_bufftail = 0;
bool got_esc = false;

// Read data from serial buffer
char serRead()
{
    if (ser_buffhead == ser_bufftail)
       return -1;
    char achar = ser_buff[ser_buffhead];
    ser_buffhead = (ser_buffhead + 1) % SERBUFFSIZE;
    return achar;
}

// Return number of characters in serial buffer
uint8_t serBuffAvailable(){
    return (ser_bufftail + SERBUFFSIZE - ser_buffhead) % SERBUFFSIZE;
}

bool checkMacro(char c){
    if(macros == false) return false;
    int num = (int)(c - '0');
    if(int(c) == 27){
        got_esc = true;
        return true;
    }    
    if(got_esc == true && num >= 0 && num <= 9){
        sendMacro(num);
        got_esc = false;
        return true;
    }
    else{
        got_esc = false;
        return false;
    }
}

void sendMacro(int num){
    char mychar;
    int c;
    
    for(c = 0; c < MACRO_MAX_LEN; c++){
        mychar = char(EEPROM[MACRO_START_ADDR + (num * MACRO_MAX_LEN) + c]);
        if(mychar != '>' && mychar != '\0' && mychar != '*'){
              sendMacroChar(mychar);
        }
        else if(mychar == '>'){    // CRLF
              sendMacroChar('\r');
              sendMacroChar('\n');                 
        }
        else if(mychar == '*' || mychar == '\0') break;
    }
}

void sendMacroChar(char ch){
    if(machine_type == 32)
        rttySendChar(ch);
    else // M33
        ascii_rttySendChar(ch);
    HostSerial.print(ch);
        if(echo == true)
            ttySendCharX(ch); // Special routine for echoing macros
}

// Main loop tx/rx routine
void doTxRx()
{
    char rttych, ttych, ch;
    uint8_t chars_available = 0;
  
    if(machine_type == 32) 
        chars_available = rttyBuffAvailable();
    else // M33
        chars_available = ascii_rttyBuffAvailable();
    
    // Received from RTTY
    if(chars_available > 0){
        if(machine_type == 32)
            rttych = rttyRead();
        else // M33
            rttych = ascii_rttyRead();
        // handle LF without CR and linewrap
        ttyFilter(rttych);
    }
    // Typed on Teletype
    if(ttyBuffAvailable() && tty_isSending == false && ascii_rtty_isSending == false){
        ttych = ttyRead();
        if(!checkMacro(ttych)){
          HostSerial.print(ttych);
          // Send to rtty.
          if(machine_type == 32)
              rttySendChar(ttych);
          else // M33
              ascii_rttySendChar(ttych);
        }
    }
    // Typed on computer keyboard or host serial port attached device
    if(HostSerial.available()){
        ch = HostSerial.read();
        if(isEscapeChar(ch)){
          drawMainMenu();
          return;
        }
        HostSerial.print(ch);
        // Send to rtty.
        if(machine_type == 32)
            rttySendChar(ch);
        else // M33
            ascii_rttySendChar(ch);
        // Send to teletype
        ttySendChar(ch);
    }
}

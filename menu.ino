
/* *****************************************************************************
 *  Menu Routines - View/Set machine type, baud, reader control, echo, uos, crlf, linewrap, debug
 *  Enter/Exit with '*' or ESC
*  *****************************************************************************/
#include <EEPROM.h>

#define INTXRX 0
#define INMAINMENU 1
#define INMTMENU 2
#define INBAUDMENU 3
#define INRCMENU 4
#define INECHOMENU 5
#define INUOSMENU 6
#define INDEBUGMENU 7
#define INMACROMENU 8
#define INLISTMACROMENU 9
#define INEDITMACROMENU 10
#define INTOGGLEMACROSMENU 11
#define INNOMENU 12
#define INCRLFMENU 13
#define INLINEWRAPMENU 14
#define INRTDDELAYMENU 15
#define ESC 27
#define RDR_CTS 1
#define RDR_NONE 0
#define RDR_BUFFER 2
#define MACRO_START_ADDR 300
#define MACRO_MAX_LEN 72

int rtty_baud = 45;
char var;
String str;
int8_t prog_state;
String esc_to_quit_string = "ESC to quit";
String on_string = "ON", off_string = "OFF";

// EEPROM locations
int16_t ee_machine_type = 0, ee_baud = 1, ee_reader_control = 2, ee_debug = 3, ee_echo = 4, ee_uos = 5, ee_crlf = 6, ee_linewrap = 7;
int16_t ee_rtsdelay1 = 8, ee_rtsdelay2 = 9, ee_macros = 10;
//
int8_t reader_control, debug, crlf, linewrap, macros;
int16_t rtsdelay;

void drawMainMenu(){
    HostSerial.println();
    HostSerial.println("Main Menu. " + esc_to_quit_string);
    HostSerial.print(F("1. Machine Type: "));
    HostSerial.println(machine_type);
    HostSerial.print(F("2. Baud: "));
    if(machine_type == 32)
        HostSerial.println(rtty_baud);
    else
        HostSerial.println(F("110"));
    HostSerial.print(F("3. Reader Control: "));
    if(reader_control == 1) 
        HostSerial.println(F("CTS"));
    else if(reader_control == 0)
        HostSerial.println(F("None"));
    else if(reader_control == 2)
        HostSerial.println(F("TTY Buffer"));
    HostSerial.print(F("4. Echo: "));
    if(echo == true) 
        HostSerial.println(on_string);
    else
        HostSerial.println(off_string);
    HostSerial.print(F("5. UOS: "));
    if(uos == true) 
        HostSerial.println(on_string);
    else
        HostSerial.println(off_string);
    HostSerial.print(F("6. CRLF: "));
    if(crlf == true) 
        HostSerial.println(on_string);
    else
        HostSerial.println(off_string);
    HostSerial.print(F("7. Line Wrap: "));
    if(linewrap == 0)
        HostSerial.println(off_string);
    else
        HostSerial.println(linewrap);    
    HostSerial.print(F("8. Debug: "));
    if(debug == true) 
        HostSerial.println(on_string);
    else
        HostSerial.println(off_string);
    HostSerial.print(F("9. Macros: "));
    if(macros == true) 
        HostSerial.println(on_string);
    else
        HostSerial.println(off_string);
    prog_state = INMAINMENU;
}

void drawMachineTypeMenu(){
    HostSerial.println();
    HostSerial.println("Select Machine Type. " + esc_to_quit_string);
    HostSerial.println(F("1. Model 33"));
    HostSerial.println(F("2. Model 32"));
    prog_state = INMTMENU;
}

void drawBaudMenu(){
    if(machine_type == 32){
        HostSerial.println();
        HostSerial.println("Select baud rate. " + esc_to_quit_string);
        HostSerial.println(F("1. 45"));
        HostSerial.println(F("2. 50"));
        HostSerial.println(F("3. 75"));
        HostSerial.println(F("4. 100"));
        prog_state = INBAUDMENU;
    }
    else {
        HostSerial.println();
        HostSerial.println(F("Model 33 baud rate fixed at 110 baud."));
        prog_state = INNOMENU;
    }
}

void drawReaderControlMenu(){
    HostSerial.println();
    HostSerial.println("Select Reader Control Type. " + esc_to_quit_string);
    HostSerial.println(F("1. CTS Reader Control"));
    HostSerial.println(F("2. No Reader Control - reader always enabled"));
    HostSerial.println(F("3. TTY Buffer Reader Control"));
    prog_state = INRCMENU;
}

void drawEchoMenu(){
    HostSerial.println();
    HostSerial.print(F("Toggle Echo On/off. Current state: "));
    if(echo == true)
        HostSerial.println("Echo " + on_string);
    else
        HostSerial.println("Echo " + off_string);
    HostSerial.println("Press space to toggle. " + esc_to_quit_string);
    prog_state = INECHOMENU;  
}

void drawUosMenu(){
    HostSerial.println();
    HostSerial.print(F("Toggle Unshift On Space On/off. Current state: "));
    if(uos == true)
        HostSerial.println("UOS " + on_string);
    else
        HostSerial.println("UOS " + off_string);
    HostSerial.println("Press space to toggle. " + esc_to_quit_string);
    prog_state = INUOSMENU;  
}

void drawCrLfMenu(){
    HostSerial.println();
    HostSerial.print(F("Toggle LF to CRLF On/off. Current state: "));
    if(crlf == true)
        HostSerial.println("LF->CRLF " + on_string);
    else
        HostSerial.println("LF->CRLF " + off_string);
    HostSerial.println("Press space to toggle. " + esc_to_quit_string);
    prog_state = INCRLFMENU;  
}

void drawDebugMenu(){
    HostSerial.println();
    HostSerial.print(F("Enable/Disable printing figs & ltrs characters as < & >. "));
    if(debug == true)
        HostSerial.println("Debug " + on_string);
    else
        HostSerial.println("Debug " + off_string);
    HostSerial.println("Press space to toggle. " + esc_to_quit_string);
    prog_state = INDEBUGMENU;  
}

void drawMacroMenu(){
    HostSerial.println();
    HostSerial.println("Select Option. " + esc_to_quit_string);
    HostSerial.print(F("1. Enable/Disable Macros. Current state: "));
        if(macros == true)
        HostSerial.println("Macros " + on_string);
    else
        HostSerial.println("Macros " + off_string);
    HostSerial.println(F("2. List Macros"));
    HostSerial.println(F("3. Edit Macros"));
    prog_state = INMACROMENU;
}

void drawLineWrapMenu(){
    HostSerial.println();
    HostSerial.println("Select line wrap position. " + esc_to_quit_string);
    HostSerial.println(F("1. No Line Wrap"));
    HostSerial.println(F("2. 68"));
    HostSerial.println(F("3. 72"));
    prog_state = INLINEWRAPMENU;
}

void drawToggleMacrosMenu(){
    HostSerial.println();
    HostSerial.print(F("Enable/Disable Macros. Current state: "));
    if(macros == true)
        HostSerial.println("Macros " + on_string);
    else
        HostSerial.println("Macros " + off_string);
    HostSerial.println("Press space to toggle. " + esc_to_quit_string);
    prog_state = INTOGGLEMACROSMENU;  
}

boolean isEscapeChar(char c){
    //if(c == '*' || c == ESC)
    if(c == ESC)
        return true;
    else
        return false;
}

void toggleEcho(){
    if(echo == false){
        echo = true;
        EEPROM.update(ee_echo, 1);
        HostSerial.println();
        HostSerial.println("Echo " + on_string);
    }
    else {
        echo = false;
        EEPROM.update(ee_echo, 0);
        HostSerial.println();
        HostSerial.println("Echo " + off_string);
    }
}

void toggleUos(){
    if(uos == false){
        uos = true;
        EEPROM.update(ee_uos, 1);
        HostSerial.println();
        HostSerial.println("UOS " + on_string);
    }
    else {
        uos = false;
        EEPROM.update(ee_uos, 0);
        HostSerial.println();
        HostSerial.println("UOS " + off_string);
    }
}

void toggleCrLf(){
    if(crlf == false){
        crlf = true;
        EEPROM.update(ee_crlf, 1);
        HostSerial.println();
        HostSerial.println("CRLF " + on_string);
    }
    else {
        crlf = false;
        EEPROM.update(ee_crlf, 0);
        HostSerial.println();
        HostSerial.println("CRLF " + off_string);
    }
}

void toggleDebug(){
    if(debug == false){
        debug = true;
        EEPROM.update(ee_debug, 1);
        HostSerial.println();
        HostSerial.println("Debug " + on_string);
    }
    else {
        debug = false;
        EEPROM.update(ee_debug, 0);
        HostSerial.println();
        HostSerial.println("Debug " + off_string);
    }
}

void toggleMacros(){
    if(macros == false){
        macros = true;
        EEPROM.update(ee_macros, 1);
        HostSerial.println();
        HostSerial.println("Macros " + on_string);
    }
    else {
        macros = false;
        EEPROM.update(ee_macros, 0);
        HostSerial.println();
        HostSerial.println("Macros " + off_string);
    }
}

void listMacros(){
    listMacros1();
    prog_state = INLISTMACROMENU;
    HostSerial.print(esc_to_quit_string);
}

void listMacros1(){
    char mychar;
    bool newline = false;
    HostSerial.println();
    HostSerial.println();
    for(int j = 0; j < 10; j++){
        newline = false;
        HostSerial.print(j); HostSerial.print(". ");
        for(int c = 0; c < MACRO_MAX_LEN; c++){
            mychar = char(EEPROM[MACRO_START_ADDR + (j * MACRO_MAX_LEN) + c]);
            HostSerial.print(mychar);
            if(mychar == '\n') newline = true;
        }
        if(newline == false)
            HostSerial.println();;
    }
}

void editMacros(){
    char c;
    listMacros1();
    HostSerial.print(F("Select Macro to edit (0-9): "));
    HostSerial.print(F("CRLF = '>'; Press '*' to end edit."));
    while (Serial.available() == 0);  // Wait for character
    while (Serial.available() > 0) {
        c = Serial.read();
    }
    HostSerial.println(c);
    prog_state = INEDITMACROMENU;
    if(c >= '0' and c <= '9')
        editMacro(int(c - '0'));
    HostSerial.println();
    HostSerial.print(esc_to_quit_string);
}

void editMacro(int num){
    char ser_buff[MACRO_MAX_LEN];
    char c = '\0';
    int i = 0, j = 0;
    while( c != '*' && i < MACRO_MAX_LEN) {
        while (Serial.available() == 0);  // Wait for character
        while (Serial.available() > 0) {  // then read it
            c = Serial.read();
            if (c == 8 || c == 127){    // Process backspace or delete
                Serial.print(char(8));
                Serial.print(' ');
                Serial.print(char(8));
                i--;
                if(i < 0) i=0;
            }
            else if(c == '\r' || c == '\n')   // Ignore CRLF
                break;
            else{
                Serial.print(char(toupper(c)));        // Process character
                ser_buff[i] = char(toupper(c));
                i++;
            }
            if(c == '*'){    // We're finished editing - store it.
                Serial.println();
                for(j = 0; j < i; j++){
                    Serial.print(ser_buff[j]);
                    EEPROM[MACRO_START_ADDR + (num * MACRO_MAX_LEN) + j] = ser_buff[j];
                }
                Serial.println();
                for(int k = j; k < MACRO_MAX_LEN; k++){
                    EEPROM[MACRO_START_ADDR + (num * MACRO_MAX_LEN) + k] = 0;
                }
            }
        }
    }
    if( i >= MACRO_MAX_LEN ){
        Serial.println();
        Serial.println(F("Max macro length (72) exceeded."));
    }
    return;
}
    
void initMenu(){
  prog_state = INTXRX;
}

// Get machine values from eeprom. If value read is not valid, set default.
// Defaults: machine_type = 33, baud = 45, reader_control = CTS, debug = false, echo = false, uos = false
void get_eeprom_values(void){
    machine_type = EEPROM.read(ee_machine_type);
    if(machine_type != 33 and machine_type != 32) machine_type = 33;
    rtty_baud = EEPROM.read(ee_baud);
    if(rtty_baud != 45 and rtty_baud != 50 and rtty_baud != 75 and rtty_baud != 100) rtty_baud = 45;
    if(rtty_baud == 45) setRttyBittime(22);
    else if(rtty_baud == 50) setRttyBittime(20);
    else if(rtty_baud == 75) setRttyBittime(13);
    else if(rtty_baud == 100) setRttyBittime(10);
    reader_control = EEPROM.read(ee_reader_control);
    if(reader_control != 1 and reader_control != 0 and reader_control != 2) reader_control = 1;
    debug = EEPROM.read(ee_debug);
    if(debug != 1 and debug != 0) debug = 0;
    echo = EEPROM.read(ee_echo);
    if(echo != 1 and echo != 0) echo = 0;
    uos = EEPROM.read(ee_uos);
    if(uos != 1 and uos != 0) uos = 0;
    crlf = EEPROM.read(ee_crlf);
    if(crlf != 1 and crlf != 0) crlf = 0;
    linewrap = EEPROM.read(ee_linewrap);
    if(linewrap != 0 and linewrap != 68 and linewrap != 72) linewrap = 0;
    macros = EEPROM.read(ee_macros);
    if(macros != 1 and macros != 0) macros = 0;
}

boolean inMenu(){
  if(prog_state == INTXRX)
      return false;
  else
      return true;
}

void menuProcess(){
    var = '\0';
    if(HostSerial.available()){
        var = HostSerial.read();
        if(var >= ' ')
          HostSerial.print(var);
    }
    if(prog_state == INMAINMENU){
        switch(var){
            case '1':
              drawMachineTypeMenu();
              break;
            case '2':
              drawBaudMenu();
              break;
            case '3':
              drawReaderControlMenu();
              break;
            case '4':
              drawEchoMenu();
              break;
            case '5':
              drawUosMenu();
              break;
            case '6':
              drawCrLfMenu();
              break;
            case '7':
              drawLineWrapMenu();
              break;
            case '8':
              drawDebugMenu();
              break;
            case '9':
              drawMacroMenu();
              break;
            case '*':
            case ESC:
              HostSerial.println();
              HostSerial.println(F("Exit Menu. In TX/RX mode."));
              prog_state = INTXRX;
              break;
        }
    }
    else if(prog_state == INMTMENU){
          switch(var){
            case '1':
                machine_type = 33;
                EEPROM.update(ee_machine_type, 33);
                drawMainMenu();
                break;
            case '2':
                machine_type = 32;
                EEPROM.update(ee_machine_type, 32);
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INRCMENU){
          switch(var){
            case '1':
                reader_control = 1; // CTS
                EEPROM.update(ee_reader_control, 1);
                drawMainMenu();
                break;
            case '2':
                reader_control = 0; // None
                EEPROM.update(ee_reader_control, 0);
                drawMainMenu();
                break;
            case '3':
                reader_control = 2; // Buffer
                EEPROM.update(ee_reader_control, 2);
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INBAUDMENU){
         switch(var){
            case '1':
                rtty_baud = 45;
                setRttyBittime(22);
                EEPROM.update(ee_baud, 45);
                drawMainMenu(); 
                break;
            case '2':
                rtty_baud = 50;
                setRttyBittime(20);
                EEPROM.update(ee_baud, 50);
                drawMainMenu(); 
                break;
            case '3':
                rtty_baud = 75;
                drawMainMenu();
                setRttyBittime(13);
                EEPROM.update(ee_baud, 75);
                break;
            case '4':
                rtty_baud = 100;
                setRttyBittime(10);
                EEPROM.update(ee_baud, 100);
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu(); 
                break;
        }
    }
    else if(prog_state == INECHOMENU){
          switch(var){
            case ' ':
                toggleEcho();
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INUOSMENU){
          switch(var){
            case ' ':
                toggleUos();
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INCRLFMENU){
          switch(var){
            case ' ':
                toggleCrLf();
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INLINEWRAPMENU){
          switch(var){
            case '1':
                linewrap = 0; // CTS
                EEPROM.update(ee_linewrap, 0);
                drawMainMenu();
                break;
            case '2':
                linewrap = 68; // None
                EEPROM.update(ee_linewrap, 68);
                drawMainMenu();
                break;
            case '3':
                linewrap = 72; // Buffer
                EEPROM.update(ee_linewrap, 72);
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INDEBUGMENU){
          switch(var){
            case ' ':
                toggleDebug();
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INMACROMENU){
          switch(var){
            case '1':
                drawToggleMacrosMenu();
                break;
            case '2':
                listMacros();
                break;
            case '3':
                editMacros();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }
    }      
    else if(prog_state == INLISTMACROMENU){
          switch(var){
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INEDITMACROMENU){
          switch(var){
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INTOGGLEMACROSMENU){
          switch(var){
            case ' ':
                toggleMacros();
                drawMainMenu();
                break;
            case '*':
            case ESC:
                drawMainMenu();
                break;
        }      
    }
    else if(prog_state == INNOMENU)
        drawMainMenu();
}

void eepromUpdateInt(int16_t num, int16_t addr){
    EEPROM.update(addr, highByte(num));
    EEPROM.update(addr + 1, lowByte(num));
}

int16_t eepromReadInt(int16_t addr){
    byte high = EEPROM.read(addr);
    byte low = EEPROM.read(addr + 1);
    return word(high, low);
}

/****************************************************************
   45 Baud RTTY TX/RX for Model 33 Teletype
   Uses Reader Control to pulse Reader Trip Magnet
   Uses either CTS protocol or 45 baud pulses for reader control
  /****************************************************************/
// Version 0.1 - 2018.05.22 W.C.Wuttke

#define HostSerial Serial
#define SerialRate 9600
#define CommPortSerialRate 300
// UART receiver defines
#define SPACE 0
#define MARK 1
#define HI 1
#define LO 0
#define WAITING 0
#define INSTARTBIT 1
#define INDATA 2
#define XON 17
#define XOFF 19

#include <FlexiTimer2.h>
#include <SoftwareSerial.h>
#include "macros.h"

bool ttytxbe;
bool echo;
bool uos;
bool tty_isSending = false;
bool ascii_rtty_isSending = false;
int8_t machine_type;
int8_t ttytxbuf;
int rts_timeout = 2500;

//#define QBFMSG "THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG'S BACK 1234567890 TIMES."
//#define NOWMSG "NOW IS THE TIME FOR ALL GOOD MEN TO COME TO THE AID OF THEIR COUNTRY."

/* *****************************************************************************
    INTERRUPT ROUTINE
*  *****************************************************************************/
// Main interrupt routine - calls rtty & tty handlers
// Called every 1 millisecond upon FlexiTimer2 interrupt
void interrupter(void) {
  if (machine_type == 32)
    rttyRxHandler();
  else if (machine_type == 33) {
    ascii_rttyReaderControl(); // Check CTS
    ascii_rttyRxHandler();
  }
  ttyTxHandler();
  ttyRxHandler();
  rttyRTSTimer();
}

void setup()
{
  get_eeprom_values();
  // Digital pins
  ttyInit();        // Initializes TTY pins
  ascii_rttyInit(); // Initializes pins for both RTTY and ASCII_RTTY
  // Serial port
  HostSerial.begin(SerialRate);
  while (!HostSerial) { }
  // set up UART 1 millisecond timer
  FlexiTimer2::set(1, interrupter);
  FlexiTimer2::start();
  // Start off in TX/RX mode
  initMenu();
}

void loop() {
  if (!inMenu())
      doTxRx();
  else
      menuProcess();
}

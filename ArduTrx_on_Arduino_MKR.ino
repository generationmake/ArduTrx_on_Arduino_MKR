/* Version for Arduino MKR WIFI 1010
 *  
 * software for arduino shield ArduTrx with Dorji or NiceRF HF modules
 * http://ardutrx.generationmake.de
 * bernhard@generationmake.de
 * 
 * supported HF modules:
 *     - NiceRF SA818-V (134 - 174 MHz) http://www.nicerf.com/product_151_104.html
 *     - NiceRF SA818-U (400 - 480 MHz) http://www.nicerf.com/product_151_104.html
 *     - Dorji DRA818V (134 - 174 MHz) http://www.dorji.com/docs/data/DRA818V.pdf
 *     - Dorji DRA818U (400 - 470 MHz) http://www.dorji.com/docs/data/DRA818U.pdf
 *
 * Additional libraries (need to be installed in Arduino)
 * TimerOne
 * FlashStorage
 *
 ************* old history - just for information - may be deleted! ******************
 * Version 0.1   - 16.05.2016 - initial version
 * Version 0.2   - 19.05.2016 - corrected frequency string to DRA818
 *                            - set PD, H/L and PTT to definded levels
 *                            - added 1750 Hz tone
 *                            - added rx/tx split from 145.600 to 145.800
 * Version 0.3   - 25.03.2018 - corrected 1750 Hz tone: now it stops after releasing the select button
 *                            - added boot up message with version
 *                            - added callsign
 *                            - updated menu and selection of options
 *                            - added software switch to change power level
 *                            - display rx/squelch in display (* at last position)
 * Version 0.4   - 27.03.2018 - added power level switch - by Mathias Metzner DH7AHO
 *                            - added defines for input and output pins
 * Version 0.5   - 04.04.2018 - added functions to store data in eeprom
 *                            - simplified encoder switch functions
 *                            - pullups on encoder inputs
 *                            - restore factory settings when encoder pressed during start up
 * Version 0.6   - 06.04.2018 - added simple menu
 *                            - menu option for factory setting
 *                            - filter can be set
 *                            - added strings for ctcss and on/off
 * Version 0.7   - 25.04.2018 - scan frequencies
 *                            - defines for tune and split limit
 * Version 0.8   - 27.04.2018 - check communication with dra818 with handshake command
 *                            - now compatible with Arduino Leonardo
 *                            - changed encoder from pin change interrupt to timer interrupt because Arduino Leonardo doesn't support pin change interrupt on these pins
 * Version 0.9   - 13.08.2018 - support for DRA818U
 *                            - changed type of frequency variable to unsigned to have enough range for 70 cm frequencies
 * Version 0.10  - 23.11.2018 - support for NiceRF SA818 HF modules
 *                            - SA818 function: display version of module
 *                            - SA818 function: display RSSI in menu and on main screen during receive
 * Version 0.11  - 24.11.2018 - SA818 function: configure tail tone
 *                            - measure input voltage
 *                            - shutdown at undervoltage
 *                    
 */

#define MY_CALLSIGN "ArduTrx"            // callsign here will display on line 1 
// select your HF module
//#define DRA818U     // support for DRA818U; leave uncommented for DRA818V
//#define SA818V     // support for SA818-V; leave uncommented for DRA818V
#define SA818U     // support for SA818-U; leave uncommented for DRA818V

#include <DogGraphicDisplay.h>
#include "ubuntumono_b_32.h"
#include "ubuntumono_b_16.h"
#include "ubuntumono_b_8.h"
DogGraphicDisplay DOG;
//#include <TimerOne.h>

// define inputs and outputs
#define IN_SQ   2
#define OUT_MIC 3
#define OUT_BACKLIGHT A6
#define OUT_PTT 4
#define OUT_PD  5
#define OUT_H_L 7
// define encoder pins
#define IN_encoder0PinA  A3
#define IN_encoder0PinB  A4
#define IN_encoder0PinSW A5
// define menu
#if defined(SA818V) || defined(SA818U)
  #define MENU_LENGTH 13
#else
  #define MENU_LENGTH 10
#endif
// define frequencies
#if defined(DRA818U) || defined(SA818U)
// define frequencies for DRA818U and SA818-U
  #define SCAN_LIMIT_LOWER 34400  // 430.000 MHz
  #define SCAN_LIMIT_UPPER 35200  // 440.000 MHz
  #define TUNE_LIMIT_LOWER 32000  // 400.000 MHz
  #define TUNE_LIMIT_UPPER 37600  // 470.000 MHz
  #define SPLIT_LIMIT_LOWER 35084 // 438.550 MHz
  #define SPLIT_LIMIT_UPPER 35155 // 439.4375 MHz
  #define SPLIT_DIFF 608          // 7.600 MHz
#else
// define frequencies for DRA818V and SA818-V
  #define SCAN_LIMIT_LOWER 11520  // 144.000 MHz
  #define SCAN_LIMIT_UPPER 11680  // 146.000 MHz
  #define TUNE_LIMIT_LOWER 10720  // 134.000 MHz
  #define TUNE_LIMIT_UPPER 13920  // 174.000 MHz
  #define SPLIT_LIMIT_LOWER 11648 // 145.600 MHz
  #define SPLIT_LIMIT_UPPER 11664 // 145.800 MHz
  #define SPLIT_DIFF 48           // 0.600 MHz
#endif

//compatibility for arduino leonardo
// leonardo uses Serial1
#ifdef __AVR_ATmega32U4__
  #define SerialDra Serial1
#else // arduino uno
  #define SerialDra Serial
#endif

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

//user parameters, will be stored in eeprom
struct userparameters
{
  byte ardutrx_version;     // version identifier
#if defined(DRA818U) || defined(SA818U)
  unsigned int encoder0Pos = 35124;      // start frequency 439.050MHz
#else
  unsigned int encoder0Pos = 11654;      // start frequency 145.675MHz
#endif
  byte vol=5;                // volume level
  byte sql=5;                // squelch level
  byte power_level=0;       // power level
  byte ctcss=10;            // ctcss 67, 71.9, 74.4, 77, 79.7, 82.5, 85.4, 88.5, 91.5,  94.8, 97.4, 100, 103.5, 107.2, 110.9, 114.8, 118.8, 123,   127.3, 131.8, 136.5, 141.3, 146.2, 151.4, 156.7, 162.2, 167.9,  173.8, 179.9, 186.2, 192.8, 203.5, 210.7, 218.1, 225.7, 233.6,241.8, 250.3
  byte filter_pre_de_emph=0;
  byte filter_highpass=0;
  byte filter_lowpass=0;
  byte tail_tone=0;
  unsigned int undervoltage=0;
};
typedef userparameters userparameters_t;

struct userparameters u;

//#include <EEPROM.h>   // eeprom library for settings
#include <FlashStorage.h>   // eeprom library for settings
FlashStorage(storedparameters, userparameters_t);

//variables for transceiver
int update=1; // update of frequency and suelch necessary
int update_filter=1;    // update of filter settings necessary
int update_tail_tone=1; // update of tail tone settings necessary
int sel=0;  // pointer for menu
byte menu_in=0;  // in menu?
unsigned long last_settings_update;   // variable to store when setting were changed last.

// Set timer TC4 to call the TC4_Handler every 5ms: (8 * (29999 + 1)) / 48MHz = 5ms
void init_timer() {
  PORT->Group[PORTA].DIRSET.reg = PORT_PA21;               // Set D7 as a digital output
 
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |                 // Enable GCLK0 for TC4 and TC5
                      GCLK_CLKCTRL_GEN_GCLK0 |             // Select GCLK0 at 48MHz
                      GCLK_CLKCTRL_ID_TC4_TC5;             // Feed GCLK0 output to TC4 and TC5
  while (GCLK->STATUS.bit.SYNCBUSY);                       // Wait for synchronization
 
  TC4->COUNT16.CC[0].reg = 29999;                          // Set the TC4 CC0 register as the TOP value in match frequency mode
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);                // Wait for synchronization

  NVIC_SetPriority(TC4_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for TC4 to 0 (highest)
  NVIC_EnableIRQ(TC4_IRQn);         // Connect TC4 to Nested Vector Interrupt Controller (NVIC)

  TC4->COUNT16.INTENSET.reg = TC_INTENSET_OVF;             // Enable TC4 overflow (OVF) interrupts
 
  TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCSYNC_PRESC |     // Reset timer on the next prescaler clock
                            TC_CTRLA_PRESCALER_DIV8 |      // Set prescaler to 8, 48MHz/8 = 6MHz
                            TC_CTRLA_WAVEGEN_MFRQ |        // Put the timer TC4 into match frequency (MFRQ) mode
                            TC_CTRLA_MODE_COUNT16;         // Set the timer to 16-bit mode     
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);                // Wait for synchronization

  TC4->COUNT16.CTRLA.bit.ENABLE = 1;                       // Enable the TC4 timer
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);                // Wait for synchronization
}

// read the buttons
int read_LCD_buttons()
{
  adc_key_in = analogRead(0);      // read the value from the sensor 
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 250)  return btnUP; 
  if (adc_key_in < 450)  return btnDOWN; 
  if (adc_key_in < 650)  return btnLEFT; 
  if (adc_key_in < 850)  return btnSELECT;  

  return btnNONE;  // when all others fail, return this...
}

//set frequency and squelch of dra818 
void send_dra(char *frxbuffer, char *ftxbuffer, int squ, byte ctcss)
{
  SerialDra.print("AT+DMOSETGROUP=0,");         // begin message
  SerialDra.print(ftxbuffer);
  SerialDra.print(",");
  SerialDra.print(frxbuffer);
  SerialDra.print(",00");
  if(ctcss<10) SerialDra.print("0");   // arduino generates no leading zeros
  SerialDra.print(ctcss);            // print ctcss
  SerialDra.print(",");    
  SerialDra.print(squ);
  SerialDra.println(",0000");
}

//set volume of dra818
void send_dravol(int vol)
{
  SerialDra.print("AT+DMOSETVOLUME=");         // begin message
  SerialDra.println(vol);
}
//set filter of dra818
void send_drafilter(byte pre_de_emph, byte highpass, byte lowpass)
{
  SerialDra.print("AT+SETFILTER=");         // begin message
  SerialDra.print(pre_de_emph);
  SerialDra.print(",");
  SerialDra.print(highpass);
  SerialDra.print(",");
  SerialDra.println(lowpass);
}
//send scan command to dra818 
byte send_dra_scan(char *frqbuffer)
{
  char rxbuffer[10];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  do
  {
    SerialDra.print("S+");         // begin message
    SerialDra.println(frqbuffer);
    rxlen=SerialDra.readBytesUntil('\n',rxbuffer,4);
  } while(rxlen==0);    // send command until answer is received
  if(rxlen==4) rxbuffer[rxlen-1]=0;  // check length of answer and remove cr character
  rxbuffer[rxlen]=0; // remove last byte and end string
  if(rxbuffer[0]=='S') // check if answer starts with S
  {
    if(rxbuffer[2]=='0') return 0;   // there is signal on this frequency
    else if(rxbuffer[2]=='1') return 1;  // there is no signal on this frequency
    else return -1;   // something went wrong
  }
  else return -1; // something went terribly wrong
}

//send handshake command to dra818 
void send_dra_handshake(void)
{
  char rxbuffer[20];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  do
  {
    SerialDra.println("AT+DMOCONNECT");         // begin message
    rxlen=SerialDra.readBytesUntil('\n',rxbuffer,19);
  } while(rxlen==0);    // send command until answer is received
  rxbuffer[rxlen-1]=0;  // check length of answer and remove cr character
  rxbuffer[rxlen]=0; // remove last byte and end string
  DOG.string(0,0,UBUNTUMONO_B_16,rxbuffer,ALIGN_CENTER,STYLE_FULL);
  delay(1000);    // wait a little bit
}

#if defined(SA818V) || defined(SA818U)
//set tail tone of sa818
void send_dra_tail_tone(int vol)
{
  SerialDra.print("AT+SETTAIL=");         // begin message
  SerialDra.println(vol);
}
//send version command to sa818 
void send_dra_version(void)
{
  char rxbuffer[25];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  do
  {
    SerialDra.println("AT+VERSION");         // begin message
    rxlen=SerialDra.readBytesUntil('\n',rxbuffer,24);
  } while(rxlen==0);    // send command until answer is received
  rxbuffer[rxlen-1]=0;  // check length of answer and remove cr character
  rxbuffer[rxlen]=0; // remove last byte and end string
  if(rxlen>9) DOG.string(0,0,UBUNTUMONO_B_16,rxbuffer+9,ALIGN_CENTER,STYLE_FULL);
  delay(1000);    // wait a little bit
}

//send RSSI command to sa818 
void send_dra_rssi(void)
{
  char rxbuffer[20];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  serial_in_flush();
  do
  {
    SerialDra.println("RSSI?");         // begin message
    rxlen=SerialDra.readBytesUntil('\n',rxbuffer,19);
  } while(rxlen==0);    // send command until answer is received
  rxbuffer[rxlen-1]=0;  // check length of answer and remove cr character
  rxbuffer[rxlen]=0; // remove last byte and end string
  if(rxlen>6) DOG.string(0,0,UBUNTUMONO_B_16,rxbuffer+5,ALIGN_CENTER,STYLE_FULL);
  delay(100);    // wait a little bit
}
#endif

// set output power level of dra818
void set_power_level(byte level)
{
  if(level==1) digitalWrite(OUT_H_L,LOW); // 1 W
  else digitalWrite(OUT_H_L,HIGH); // 0,5 W
}
// scan function
byte frequency_scan(byte dir, byte scan_run)
{
  char frxbuffer[10];  // buffer for frequency string
  unsigned int freqa,freqb;    // fractals of frequency
  static byte rx=0;

  if(scan_run==1) // while scan is running
  {
    if(dir==1) u.encoder0Pos++; // increase frequency
    else u.encoder0Pos--;       // decrease frequency
    if(u.encoder0Pos<SCAN_LIMIT_LOWER) u.encoder0Pos=SCAN_LIMIT_UPPER;  // check lower limit
    if(u.encoder0Pos>SCAN_LIMIT_UPPER) u.encoder0Pos=SCAN_LIMIT_LOWER;  // check upper limit
  }
  freqa=(u.encoder0Pos/80);  // frequency integral part
  freqb=(u.encoder0Pos%80)*125;  // frequency fractional part
  sprintf(frxbuffer,"%03i.%04i",freqa,freqb);  // generate frequency string
  DOG.string(0,0,UBUNTUMONO_B_16,frxbuffer,ALIGN_CENTER,STYLE_FULL);
  if(scan_run==1) // check if scan is still running
  {
    rx=send_dra_scan(frxbuffer);  // send command to dra
  }
  if(rx==0) DOG.string(0,2,UBUNTUMONO_B_16,"stop",ALIGN_CENTER,STYLE_FULL);
  else DOG.string(0,0,UBUNTUMONO_B_16,"run",ALIGN_CENTER,STYLE_FULL);
  delay(100);   // wait a little bit
  return rx;    // return result
}
// flush serial in bufffer
void serial_in_flush(void)
{
  while(SerialDra.available())   // check if bytes available
  {
    SerialDra.read();            // and read them all
  }
}

// display and menu routines
// display active menu point inverse
void display_cursor(int sel, byte level)
{
  byte style=STYLE_NORMAL;
  if(sel==2) style=STYLE_INVERSE; // display power level inverse when selected
  if(level==1) DOG.string(0,4,UBUNTUMONO_B_16,"High",ALIGN_LEFT,style);
  else DOG.string(0,4,UBUNTUMONO_B_16,"Low ",ALIGN_LEFT,style);
  if(sel==0) DOG.string(0,6,UBUNTUMONO_B_16,"VOL",ALIGN_LEFT,STYLE_INVERSE); // volume
  else DOG.string(0,6,UBUNTUMONO_B_16,"VOL",ALIGN_LEFT,STYLE_NORMAL); // volume
  if(sel==1) DOG.string(80,6,UBUNTUMONO_B_16,"SQ",ALIGN_LEFT,STYLE_INVERSE); // squelch
  else DOG.string(80,6,UBUNTUMONO_B_16,"SQ",ALIGN_LEFT,STYLE_NORMAL); // squelch
  if(sel==3) DOG.string(0,4,UBUNTUMONO_B_16,"MENU",ALIGN_RIGHT,STYLE_INVERSE); // menu
  else DOG.string(0,4,UBUNTUMONO_B_16,"MENU",ALIGN_RIGHT,STYLE_NORMAL); // menu
}

void display_main_screen(void)
{
  DOG.clear();
  display_cursor(sel,u.power_level);   // position cursor for menu  
}
//menu functions
void display_menu(byte action)
{
  static byte menu_pointer=0; // pointer for active menu
  static byte menu_sub=0; // submenu active
  static byte scan_dir=1; // scan direction up
  static byte scan_run=0; // scan stopped
//strings
  const char* strings_ctcss[]={"none","67 Hz", "71.9 Hz", "74.4 Hz", "77 Hz", "79.7 Hz", "82.5 Hz", "85.4 Hz", "88.5 Hz", "91.5 Hz",  "94.8 Hz", "97.4 Hz", "100 Hz", "103.5 Hz", "107.2 Hz", "110.9 Hz", "114.8 Hz", "118.8 Hz", "123 Hz", "127.3 Hz", "131.8 Hz", "136.5 Hz", "141.3 Hz", "146.2 Hz", "151.4 Hz", "156.7 Hz", "162.2 Hz", "167.9 Hz", "173.8 Hz", "179.9 Hz", "186.2 Hz", "192.8 Hz", "203.5 Hz", "210.7 Hz", "218.1 Hz", "225.7 Hz", "233.6 Hz", "241.8 Hz", "250.3 Hz"};
  const char* strings_onoff[]={"on","off"};
  const char* strings_offon[]={"off","on"};
  char buffer[50];


  if(action==1) // right
  {
    if(menu_sub==0) menu_pointer++;
    else    // submenu active
    {
      if(menu_pointer==0) // scan
      {
        scan_dir=1; //up
        scan_run=1; // activate scan
      }
      if(menu_pointer==1) // ctcss
      {
        if(u.ctcss<38)
        {
          u.ctcss++;
//          update=1; // has to be reworked because now also displays frequency
        }
      }
      if(menu_pointer==2) // filter PRE/DE-EMPH
      {
        u.filter_pre_de_emph=1;
        update_filter=1;
      }
      if(menu_pointer==3) // filter highpass
      {
        u.filter_highpass=1;
        update_filter=1;
      }
      if(menu_pointer==4) // filter lowpass
      {
        u.filter_lowpass=1;
        update_filter=1;
      }
      if(menu_pointer==5) reset_factory_settings(); // factory settings
      if(menu_pointer==8) // undervoltage
      {
        if(u.undervoltage<20000)
        {
          u.undervoltage+=100;
        }
      }
#if defined(SA818V) || defined(SA818U)
      if(menu_pointer==11) // tail tone
      {
        u.tail_tone=1;
        update_tail_tone=1;
      }
#endif
    }
  }
  if(action==2) // left
  {
    if(menu_sub==0) menu_pointer+=(MENU_LENGTH-1);
    else    // submenu active
    {
      if(menu_pointer==0) // scan
      {
        scan_dir=0; //down
        scan_run=1; // activate scan
      }
      if(menu_pointer==1) // ctcss
      {
        if(u.ctcss>0)
        {
          u.ctcss--;
//          update=1; // has to be reworked because now also displays frequency
        }
      }
      if(menu_pointer==2) // filter PRE/DE-EMPH
      {
        u.filter_pre_de_emph=0;
        update_filter=1;
      }
      if(menu_pointer==3) // filter highpass
      {
        u.filter_highpass=0;
        update_filter=1;
      }
      if(menu_pointer==4) // filter lowpass
      {
        u.filter_lowpass=0;
        update_filter=1;
      }
      if(menu_pointer==5) reset_factory_settings(); // factory settings
      if(menu_pointer==8) // undervoltage
      {
        if(u.undervoltage>0)
        {
          u.undervoltage-=100;
        }
      }
#if defined(SA818V) || defined(SA818U)
      if(menu_pointer==11) // tail tone
      {
        u.tail_tone=0;
        update_tail_tone=1;
      }
#endif
    }
  }
  if(action==3) // up
  {
    menu_sub=0;   // disable submenu
    if(menu_pointer==MENU_LENGTH-1) menu_in=0;    // back to main screen
  }
  if(action==4) // down
  {
    menu_sub=1;   // enable sub menu
    if(menu_pointer==0) serial_in_flush();  // clear serial input buffer before we start scan
    if(menu_pointer==MENU_LENGTH-1) menu_in=0;    // back to main screen
  }
  menu_pointer%=MENU_LENGTH;
  if(menu_in==1)    // if menu is active
  {
    if(action!=0) // key was pressed
    {
      DOG.clear();
      if(menu_pointer==0) DOG.string(0,0,UBUNTUMONO_B_16,"Scan",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==1) DOG.string(0,0,UBUNTUMONO_B_16,"CTCSS",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==2) DOG.string(0,0,UBUNTUMONO_B_16,"Filt PRE/DE-EMPH",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==3) DOG.string(0,0,UBUNTUMONO_B_16,"Filter Highpass",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==4) DOG.string(0,0,UBUNTUMONO_B_16,"Filter Lowpass",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==5) DOG.string(0,0,UBUNTUMONO_B_16,"factory settings",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==6) DOG.string(0,0,UBUNTUMONO_B_16,"module check",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==7) DOG.string(0,0,UBUNTUMONO_B_16,"input voltage",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==8) DOG.string(0,0,UBUNTUMONO_B_16,"set under voltage",ALIGN_CENTER,STYLE_FULL); // print menu line 1
#if defined(SA818V) || defined(SA818U)
      if(menu_pointer==9) DOG.string(0,0,UBUNTUMONO_B_16,"SA818 version",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==10) DOG.string(0,0,UBUNTUMONO_B_16,"SA818 RSSI",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==11) DOG.string(0,0,UBUNTUMONO_B_16,"SA818 tail tone",ALIGN_CENTER,STYLE_FULL); // print menu line 1
      if(menu_pointer==12) DOG.string(0,0,UBUNTUMONO_B_16,"back to main",ALIGN_CENTER,STYLE_FULL); // print menu line 1
#else
      if(menu_pointer==9) DOG.string(0,0,UBUNTUMONO_B_16,"back to main",ALIGN_CENTER,STYLE_FULL); // print menu line 1
#endif
      if(menu_sub==1)
      {
        if(menu_pointer==0)
        {
          if(frequency_scan(scan_dir, scan_run)==0) scan_run=0; // stop scan if signal was found
        }
        if(menu_pointer==1) DOG.string(0,2,UBUNTUMONO_B_16,strings_ctcss[u.ctcss],ALIGN_CENTER,STYLE_FULL);
        if(menu_pointer==2) DOG.string(0,2,UBUNTUMONO_B_16,strings_onoff[u.filter_pre_de_emph],ALIGN_CENTER,STYLE_FULL);
        if(menu_pointer==3) DOG.string(0,2,UBUNTUMONO_B_16,strings_onoff[u.filter_highpass],ALIGN_CENTER,STYLE_FULL);
        if(menu_pointer==4) DOG.string(0,2,UBUNTUMONO_B_16,strings_onoff[u.filter_lowpass],ALIGN_CENTER,STYLE_FULL);
        if(menu_pointer==5) DOG.string(0,2,UBUNTUMONO_B_16,"press right",ALIGN_CENTER,STYLE_FULL);
        if(menu_pointer==6) send_dra_handshake();
        if(menu_pointer==7) DOG.string(0,2,UBUNTUMONO_B_16,itoa(analogRead(1)*29,buffer,10),ALIGN_CENTER,STYLE_FULL);
        if(menu_pointer==8) DOG.string(0,2,UBUNTUMONO_B_16,itoa(u.undervoltage,buffer,10),ALIGN_CENTER,STYLE_FULL);

#if defined(SA818V) || defined(SA818U)
        if(menu_pointer==9) send_dra_version();
        if(menu_pointer==10) send_dra_rssi();
        if(menu_pointer==11) DOG.string(0,2,UBUNTUMONO_B_16,strings_onoff[u.tail_tone],ALIGN_CENTER,STYLE_FULL);
#endif
      }
    }
    else  // no key was pressed
    {
      if(menu_sub==1)
      {
//        lcd.setCursor(0,1); // print menu line 2 if submenu is active
        if(menu_pointer==0)
        {
          if(frequency_scan(scan_dir, scan_run)==0) scan_run=0; // stop scan if signal was found
        }
//        if(menu_pointer==7) lcd.print(analogRead(1)*29);
#if defined(SA818V) || defined(SA818U)
        if(menu_pointer==10) send_dra_rssi();  // update RSSI regularly
#endif
      }      
    }
  }
  else      // go back to main screen
  {
    menu_pointer=0;   // reset menupointers
    menu_sub=0;
    update=1;     // to display frequency and enable changes
    display_main_screen();  // show main screen
  }
}

void factory_settings()
{
//  EEPROM.write(u);  // save all user parameters to EEprom
  storedparameters.write(u);
  delay(1000);
}

void reset_factory_settings()
{
  u.ardutrx_version=0;
//  EEPROM.write(u);  // destroy version; after reset default values will be used
  storedparameters.write(u);
  DOG.string(0,0,UBUNTUMONO_B_16,"press reset",ALIGN_CENTER,STYLE_FULL);
  while(1);
}

void setup()
{
  u.ardutrx_version=2;
#if defined(DRA818U) || defined(SA818U)
  u.ardutrx_version|=0x80;  // set bit 7 for DRA818U versions
#endif
// set pins
  pinMode(IN_SQ,INPUT_PULLUP); // SQ
  pinMode(OUT_BACKLIGHT,OUTPUT); // backlight low=off, high=on
  pinMode(OUT_PTT,OUTPUT); // PTT low=rx, high=tx
  pinMode(OUT_PD,OUTPUT); // PD low=sleep, high=normal
  pinMode(OUT_H_L,OUTPUT); // H_L low=1 W, high=0.5 W
  digitalWrite(OUT_BACKLIGHT,HIGH); // backlight on
  digitalWrite(OUT_PTT,LOW); // rx
  digitalWrite(OUT_PD,LOW); // normal
  digitalWrite(OUT_H_L,HIGH); // 0.5 W
  pinMode (IN_encoder0PinA,INPUT_PULLUP);  // encoder input pins with pullup to avoid problems when encoder no correctly connected
  pinMode (IN_encoder0PinB,INPUT_PULLUP);
  pinMode (IN_encoder0PinSW,INPUT_PULLUP);

// init serial
  SerialDra.begin(9600); // start serial for communication with dra818
// init display
  DOG.initialize(6,0,0,0,1,DOGM128);   //SS = 6, 0,0= use Hardware SPI, 0 = A0, 1 = RESET, EA DOGM128-6 (=128x64 dots)
  DOG.clear();  //clear whole display
  DOG.string(0,2,UBUNTUMONO_B_16,"ArduTrx - 0.11",ALIGN_CENTER,STYLE_FULL_INVERSE);
  DOG.string(0,4,UBUNTUMONO_B_16,"17.01.2020",ALIGN_CENTER);
  delay(2000);    // wait 2 seconds

// check version number of eeprom content and reset if old
//  byte old_version;
  struct userparameters old_version;
//  EEPROM.get(0, old_version); // previous sketch version
  old_version = storedparameters.read();
  if (!digitalRead(IN_encoder0PinSW) || (old_version.ardutrx_version != u.ardutrx_version)) {
    DOG.string(0,6,UBUNTUMONO_B_16,"setting defaults",ALIGN_CENTER);
    delay(2000);    // wait 2 seconds
    factory_settings();
  }

//  EEPROM.get(0,u);    // get EEprom settings
  u = storedparameters.read();

  display_main_screen();  // show main screen

//enable interrupts
//  Timer1.initialize(1000);  // activate timer with 1 ms
//  Timer1.attachInterrupt(int_timer1);
  init_timer();
}
 
void loop()
{
  char frxbuffer[10];  // buffer for rx frequency string
  char ftxbuffer[10];  // buffer for tx frequency string
  unsigned int freqtx,freqrx;
  unsigned int freqa,freqb;
  int updatevol=0;  // set to 1 when update of volume necessary
  int sqin;   // variable for squelch input
  static int sqin_old=0;  // variable for squelch input to store old value
  int press = 0;
  static int Merker = 0;    // static to save status
  char buffer[50];

// write settings to eeprom
  if(millis()>(last_settings_update+5000))  // 5 sencons past with no user action
  {
//    EEPROM.write(u);  // save all user parameters to EEprom  - checks if data in eeprom is the same, so no risc to destroy eeprom
//    storedparameters.write(u);
  }

// check undervoltage
  if(u.undervoltage>0)
  {
    if((analogRead(1)*29)<u.undervoltage)
    {
      digitalWrite(OUT_BACKLIGHT,LOW); // backlight off
      digitalWrite(OUT_PD,HIGH); // power down
    }
    else
    {
      digitalWrite(OUT_BACKLIGHT,HIGH); // backlight on
      digitalWrite(OUT_PD,LOW); // normal
    }
  }

// squelch
  sqin=digitalRead(IN_SQ);    // read squelch input
#if defined(SA818V) || defined(SA818U)
  if(sqin==0&&menu_in==0)   // squelch and no menu active
  {
//    lcd.setCursor(0,0);
//    send_dra_rssi();
  }
#endif
  if(sqin!=sqin_old)    // compare if squelch has changed
  {
    sqin_old=sqin;      // store new value of squelch
//    lcd.setCursor(15,1);    // go to last position of display
    if(sqin)
    {
//      lcd.print(" ");  // print blank if no rx
      DOG.string(0,0,UBUNTUMONO_B_16," ",ALIGN_LEFT);
#if defined(SA818V) || defined(SA818U)
      if(menu_in==0)   // no menu active
      {
//        lcd.setCursor(0,0);
//        lcd.print(MY_CALLSIGN); // print my callsign        
      }
#endif
    }
    else 
    {
//      lcd.print("*");      // print * if rx
      DOG.string(0,0,UBUNTUMONO_B_16,"*",ALIGN_LEFT);
#if defined(SA818V) || defined(SA818U)
      if(menu_in==0)   // no menu active
      {
//        lcd.setCursor(0,0);
//        lcd.print("       "); // delete my callsign        
      }
#endif
    }
//    display_cursor(sel);         // set cursor to menu position
  }

//encoder push button
  press = digitalRead(IN_encoder0PinSW);
  if((press == 0) && (Merker == 0))
  {
    if(u.power_level==0) u.power_level=1;    // output = 1W
    else u.power_level=0;    // output = 0.5W
    last_settings_update=millis();  // trigger update
    set_power_level(u.power_level); // send it to dra818
    Merker = 1;
    display_cursor(sel,u.power_level);
    delay(10);   
  }
  if((press == 1) && (Merker == 1))
  {
    Merker = 0;
    delay(10);
  }

  lcd_key = read_LCD_buttons();  // read the buttons

// menu
  switch (lcd_key)               // depending on which button was pushed, we perform an action
  {
    case btnRIGHT:               // go to next position
      {
        if(menu_in==1)          // if in menu
        {
          display_menu(1);       // send it direct to menu function
        }
        else
        {
          sel++;
          sel=sel%4;        // our menu has 4 points
          display_cursor(sel,u.power_level);
        }
        delay(200);
        break;
      }
    case btnLEFT:                // go to previous position
      {
        if(menu_in==1)          // if in menu
        {
          display_menu(2);       // send it direct to menu function
        }
        else
        {
          sel+=3;
          sel=sel%4;        // our menu has 4 points
          display_cursor(sel,u.power_level);
        }
        delay(200);
        break;
      }
    case btnUP:       // button up
      {
        if(menu_in==1)          // if in menu
        {
          display_menu(3);       // send it direct to menu function
        }
        else
        {
          if(sel==0)    // volume
          {
            if(u.vol<8)u.vol++;
            updatevol=1;
          }
          if(sel==1)    // squelch
          {
            if(u.sql<8)u.sql++;
            update=1;
          }
          if(sel==2)    // power level
          {
            u.power_level=1;    // output = 1W
            last_settings_update=millis();  // trigger update
            set_power_level(u.power_level); // send it to dra818
            display_cursor(sel,u.power_level);
          }
          if(sel==3)    // menu
          {
            menu_in=1;
            serial_in_flush();  // clear serial input buffer before we start menu
            display_menu(0);
          }
        }
        delay(200);
        break;
      }
    case btnDOWN:   // button down
      {
        if(menu_in==1)          // if in menu
        {
          display_menu(4);       // send it direct to menu function
        }
        else
        {
          if(sel==0)    // volume
          {
            if(u.vol>1)u.vol--;
            updatevol=1;
          }
          if(sel==1)    // squelch
          {
            if(u.sql>0)u.sql--;
            update=1;
          }
          if(sel==2)    // power level
          {
            u.power_level=0;    // output = 0.5W
            last_settings_update=millis();  // trigger update
            set_power_level(u.power_level); // send it to dra818
            display_cursor(sel,u.power_level);
          }
          if(sel==3)    // menu
          {
            menu_in=1;
            serial_in_flush();  // clear serial input buffer before we start menu
            display_menu(0);
          }
        }
        delay(200);
        break;
      }
    case btnSELECT:   // select button
      {
        if(menu_in==1)          // if in menu
        {
          display_menu(5);       // send it direct to menu function
          delay(200);
        }
        else
        {
          digitalWrite(OUT_PTT, HIGH);  // enable PTT
          tone(OUT_MIC,1750);   // enable 1750 Hz tone
        }
        break;
      }
    case btnNONE:
      {
        if(menu_in==1)          // if in menu
        {
          display_menu(0);       // send it direct to menu function
        }
        noTone(OUT_MIC);    // disable 1750 Hz tone again
        digitalWrite(OUT_PTT, LOW);  // disable PTT
        break;
      }
  }

  if(update==1)    // update frequency or squelch
  {
    update=0;
    last_settings_update=millis();  // trigger update
    //limit encoder values
    freqrx=u.encoder0Pos;
    if(freqrx>TUNE_LIMIT_UPPER) freqrx=TUNE_LIMIT_UPPER;  // 174.0000 MHz
    if(freqrx<TUNE_LIMIT_LOWER) freqrx=TUNE_LIMIT_LOWER;  // 134.0000 MHz
     
    if((freqrx>=SPLIT_LIMIT_LOWER)&&(freqrx<=SPLIT_LIMIT_UPPER))   // split function for relais 145.6000 - 145.8000 MHz
    {
      freqtx=freqrx-SPLIT_DIFF;   // set tx frequency 600 kHz lower
      DOG.string(0,2,UBUNTUMONO_B_16,"-",ALIGN_LEFT);
    }
    else 
    {
      freqtx=freqrx;    // set tx frequency = rx frequency
      DOG.string(0,2,UBUNTUMONO_B_16," ",ALIGN_LEFT);
    }

    freqa=(freqrx/80);  // frequency integral part
    freqb=(freqrx%80)*125;  // frequency fractional part
    sprintf(frxbuffer,"%03i.%04i",freqa,freqb);  // generate frequency string
    freqa=(freqtx/80);  // frequency integral part
    freqb=(freqtx%80)*125;  // frequency fractional part
    sprintf(ftxbuffer,"%03i.%04i",freqa,freqb);  // generate frequency string
    DOG.string(0,0,UBUNTUMONO_B_32,frxbuffer,ALIGN_CENTER,STYLE_FULL);
    send_dra(frxbuffer,ftxbuffer,u.sql,u.ctcss);    // update volume on dra818
    DOG.string(112,6,UBUNTUMONO_B_16,itoa(u.sql,buffer,10));
    display_cursor(sel,u.power_level);
  }
  if(updatevol==1)    // update volume
  {
    updatevol=0;
    last_settings_update=millis();  // trigger update
    send_dravol(u.vol);  // update volume on dra818
    DOG.string(30,6,UBUNTUMONO_B_16,itoa(u.vol,buffer,10));
    display_cursor(sel,u.power_level);
  }
  if(update_filter==1)    // update filter
  {
    update_filter=0;
    last_settings_update=millis();  // trigger update
    send_drafilter(u.filter_pre_de_emph,u.filter_highpass, u.filter_lowpass);  // update filter settings on dra818
  }
  if(update_tail_tone==1)    // update tail tone
  {
    update_tail_tone=0;
    last_settings_update=millis();  // trigger update
    send_dra_tail_tone(u.tail_tone);  // update tail tone settings on SA818
  }
}

//void int_timer1(struct tc_module *const module_inst) // handle timer interrupt for encoder
void TC4_Handler()                                         // Interrupt Service Routine (ISR) for timer TC4
{     
  //if (TC4->COUNT16.INTFLAG.bit.OVF && TC4->COUNT16.INTENSET.bit.OVF)       // Optionally check for overflow (OVF) interrupt     
  //{   
    PORT->Group[PORTA].OUTTGL.reg = PORT_PA21;             // Toggle the D7 output
    TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;             // Clear the OVF interrupt flag
  //}
//}
//void int_timer1() // handle timer interrupt for encoder
//{
  static int encoder0PinALast = HIGH;
  static int encoder0PinBLast = HIGH;
  int na = LOW;
  int nb = LOW;

  na = digitalRead(IN_encoder0PinA);
  nb = digitalRead(IN_encoder0PinB);
  if(encoder0PinALast!=na)  // check if pin has changed
  {
    if(nb==HIGH)
    {
      if(na==HIGH) u.encoder0Pos++;
      else u.encoder0Pos--;
    }
    else
    {
      if(na==HIGH) u.encoder0Pos--;
      else u.encoder0Pos++;
    }
    update=1;
  }
  encoder0PinALast = na;
  encoder0PinBLast = nb;
}  

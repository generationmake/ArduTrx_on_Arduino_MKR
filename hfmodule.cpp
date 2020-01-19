#include <Arduino.h>

#include "hfmodule.h"
//----------------------------------------------------public Functions----------------------------------------------------
//Please use these functions in your sketch

/*-----------------------------
constructor for class, not needed by Arduino but for complete class. does not do anything.
*/
Hfmodule::Hfmodule() 
{
//  draserial=Serial;
}

/*-----------------------------
destructor for class, not needed by Arduino but for complete class. Calls Arduino end function
*/
Hfmodule::~Hfmodule() 
{
  end();
}

/*-----------------------------
Arduino begin function. Forward data to initialize function
*/
//void Hfmodule::begin(Stream &xdraserial) 
void Hfmodule::begin(byte bp_squelch,byte bp_hl,byte bp_ptt,byte bp_pd) 
{
//  Hfmodule::draserial=xdraserial;
  Hfmodule::p_squelch=bp_squelch;
  Hfmodule::p_hl=bp_hl;
  Hfmodule::p_ptt=bp_ptt;
  Hfmodule::p_pd=bp_pd;
  pinMode(p_squelch,INPUT_PULLUP); // SQ
  pinMode(p_ptt,OUTPUT); // PTT low=rx, high=tx
  pinMode(p_pd,OUTPUT); // PD low=sleep, high=normal
  pinMode(p_hl,OUTPUT); // H_L low=1 W, high=0.5 W
  digitalWrite(p_ptt,LOW); // rx
  digitalWrite(p_pd,LOW); // normal
  digitalWrite(p_hl,HIGH); // 0.5 W

  Serial1.begin(9600); // start serial for communication with dra818
}

/*-----------------------------
Arduino end function. stop SPI if enabled
*/
void Hfmodule::end() 
{
}

byte Hfmodule::getsquelch() 
{
  return digitalRead(p_squelch);    // read squelch input
}

//set frequency and squelch of dra818 
void Hfmodule::setfrequencysquelch(char *frxbuffer, char *ftxbuffer, int squ, byte ctcss)
{
  Serial1.print("AT+DMOSETGROUP=0,");         // begin message
  Serial1.print(ftxbuffer);
  Serial1.print(",");
  Serial1.print(frxbuffer);
  Serial1.print(",00");
  if(ctcss<10) Serial1.print("0");   // arduino generates no leading zeros
  Serial1.print(ctcss);            // print ctcss
  Serial1.print(",");    
  Serial1.print(squ);
  Serial1.println(",0000");
}
//set volume of dra818
void Hfmodule::setvolume(int vol)
{
  Serial1.print("AT+DMOSETVOLUME=");         // begin message
  Serial1.println(vol);
}
//set filter of dra818
void Hfmodule::setfilter(byte pre_de_emph, byte highpass, byte lowpass)
{
  Serial1.print("AT+SETFILTER=");         // begin message
  Serial1.print(pre_de_emph);
  Serial1.print(",");
  Serial1.print(highpass);
  Serial1.print(",");
  Serial1.println(lowpass);
}
//send scan command to dra818 
//byte send_dra_scan(char *frqbuffer)
byte Hfmodule::scan(char *frqbuffer)
{
  char rxbuffer[10];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  do
  {
    Serial1.print("S+");         // begin message
    Serial1.println(frqbuffer);
    rxlen=Serial1.readBytesUntil('\n',rxbuffer,4);
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
//void send_dra_handshake(void)
char* Hfmodule::handshake(void)
{
  char rxbuffer[20];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  do
  {
    Serial1.println("AT+DMOCONNECT");         // begin message
    rxlen=Serial1.readBytesUntil('\n',rxbuffer,19);
  } while(rxlen==0);    // send command until answer is received
  rxbuffer[rxlen-1]=0;  // check length of answer and remove cr character
  rxbuffer[rxlen]=0; // remove last byte and end string
  return rxbuffer;
}

//#if defined(SA818V) || defined(SA818U)
//set tail tone of sa818
//void send_dra_tail_tone(int vol)
void Hfmodule::settailtone(int vol)
{
  Serial1.print("AT+SETTAIL=");         // begin message
  Serial1.println(vol);
}
//send version command to sa818 
//void send_dra_version(void)
char* Hfmodule::getversion(void)
{
  char rxbuffer[25];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  do
  {
    Serial1.println("AT+VERSION");         // begin message
    rxlen=Serial1.readBytesUntil('\n',rxbuffer,24);
  } while(rxlen==0);    // send command until answer is received
  rxbuffer[rxlen-1]=0;  // check length of answer and remove cr character
  rxbuffer[rxlen]=0; // remove last byte and end string
  if(rxlen>9) return rxbuffer+9;
  else return rxbuffer;
}

//send RSSI command to sa818 
//void send_dra_rssi(byte pos)
char* Hfmodule::getrssi(byte pos)
{
  char rxbuffer[20];  // buffer for response string
  byte rxlen=0;   // counter for received bytes
  serial_in_flush();
  do
  {
    Serial1.println("RSSI?");         // begin message
    rxlen=Serial1.readBytesUntil('\n',rxbuffer,19);
  } while(rxlen==0);    // send command until answer is received
  rxbuffer[rxlen-1]=0;  // check length of answer and remove cr character
  rxbuffer[rxlen]=0; // remove last byte and end string
  if(rxlen>6) return rxbuffer+5;
  else return rxbuffer;
}
//#endif

// set output power level of dra818
//void set_power_level(byte level)
void Hfmodule::setpowerlevel(byte level)
{
  if(level==1) digitalWrite(p_hl,LOW); // 1 W
  else digitalWrite(p_hl,HIGH); // 0,5 W
}
void Hfmodule::setpowerdown(byte level)
{
  if(level==1) digitalWrite(p_pd,HIGH); // power down
  else digitalWrite(p_pd,LOW); // normal
}
void Hfmodule::setptt(byte level)
{
  if(level==1) digitalWrite(p_ptt,HIGH); // ptt active
  else digitalWrite(p_ptt,LOW); // ptt low
}
// flush serial in bufffer
void Hfmodule::serial_in_flush(void)
{
  while(Serial1.available())   // check if bytes available
  {
    Serial1.read();            // and read them all
  }
}

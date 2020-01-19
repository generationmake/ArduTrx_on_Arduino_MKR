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
void Hfmodule::begin(byte bp_squelch) 
{
//  Hfmodule::draserial=xdraserial;
  Hfmodule::p_squelch=bp_squelch;
  pinMode(p_squelch,INPUT_PULLUP); // SQ

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

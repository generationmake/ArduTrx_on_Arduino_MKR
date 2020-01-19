#ifndef HFMODULE_H
#define HFMODULE_H


class Hfmodule
{
  public:
    Hfmodule ();
    ~Hfmodule ();
//    void begin(HardwareSerial &draserial);
    void begin(byte bp_squelch);
    void end ();
    byte getsquelch();
 

  private:
//    HardwareSerial draserial;
    byte p_squelch;
};

#endif /* HFMODULE_H */

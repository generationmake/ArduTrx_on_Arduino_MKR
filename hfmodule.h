#ifndef HFMODULE_H
#define HFMODULE_H


class Hfmodule
{
  public:
    Hfmodule ();
    ~Hfmodule ();
//    void begin(HardwareSerial &draserial);
    void begin(byte bp_squelch,byte bp_hl,byte bp_ptt,byte bp_pd);
    void end ();
    byte getsquelch();
    void setfrequencysquelch(char *frxbuffer, char *ftxbuffer, int squ, byte ctcss);
    void setvolume(int vol);
    void setfilter(byte pre_de_emph, byte highpass, byte lowpass);
    byte scan(char *frqbuffer);
    char* handshake(void);
//#if defined(SA818V) || defined(SA818U)
    void settailtone(int vol);
    char* getversion(void);
    char* getrssi(byte pos);
//#endif
    void setpowerlevel(byte level);
    void setpowerdown(byte level);
    void setptt(byte level);
    void serial_in_flush(void);
 

  private:
//    HardwareSerial draserial;
    byte p_squelch;
    byte p_hl;
    byte p_ptt;
    byte p_pd;
};

#endif /* HFMODULE_H */

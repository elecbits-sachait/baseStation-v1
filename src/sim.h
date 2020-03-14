#ifndef sim_h
#define sim_h
#include "Arduino.h"

class GSM
{
  public:
    bool call(const char *, uint16_t, bool (*)());
    bool msg(const char *, uint16_t, const char *, bool (*)());
    void inbox(bool*);
    String send_at(String, uint16_t, bool);
    void whitelist(const char *, uint8_t);
    void clear_whitelist(uint8_t);
    void check();
    String ussd_at(String, uint16_t);
    bool check_sim();
};

#endif
#ifndef rgbled_h
#define rgbled_h
#include "Arduino.h"
//#define redpin 6
#define greenpin 4
#define bluepin 5

class rgb {
  public:
    rgb();
    void red();
    void green();
    void blue();
    void yellow();
    void purple();
    void white();
    void no_color();
};
static rgb rgb_mode;

#endif
#include "rgbled.h"

rgb::rgb() {
  //pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  no_color();
}
void rgb::red() {
  //digitalWrite(redpin, HIGH);
  digitalWrite(greenpin, LOW);
  digitalWrite(bluepin, LOW);
}
void rgb::green() {
  //digitalWrite(redpin, LOW);
  digitalWrite(greenpin, HIGH);
  digitalWrite(bluepin, LOW);
}
void rgb::blue() {
  //digitalWrite(redpin, LOW);
  digitalWrite(greenpin, LOW);
  digitalWrite(bluepin, HIGH);
}
void rgb::yellow() {
  //digitalWrite(redpin, HIGH);
  digitalWrite(greenpin, HIGH);
  digitalWrite(bluepin, LOW);
}
void rgb::purple() {
  //digitalWrite(redpin, HIGH);
  digitalWrite(greenpin, LOW);
  digitalWrite(bluepin, HIGH);
}
void rgb::white() {
  //digitalWrite(redpin, HIGH);
  digitalWrite(greenpin, HIGH);
  digitalWrite(bluepin, HIGH);
}
void rgb::no_color() {
  //digitalWrite(redpin, LOW);
  digitalWrite(greenpin, LOW);
  digitalWrite(bluepin, LOW);
}
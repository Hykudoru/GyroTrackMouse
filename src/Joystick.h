#include <Vector.h>
#ifndef JOYSTICK_H
#define JOYSTICK_H

class Joystick
{
  int xPin, yPin, zPin;
public:  
  Vector2<float> axes;
  bool isPressed = 0; // 1
  bool invertH = false; 
  bool invertV = false;

  Joystick(int xPin, int yPin, int zPin, bool invertHorizontal = false, bool invertVertical = false)
  {
    this->xPin = xPin;
    this->yPin = yPin;
    this->zPin = zPin;
    invertH = invertHorizontal;
    invertV = invertVertical;
    pinMode(zPin, INPUT_PULLUP);
  }
  ~Joystick() {}
  void Start();
  Vector3<float> Read(bool normalize = false);
  void Print();
};

#endif
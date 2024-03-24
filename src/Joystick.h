#include <Vector.h>
#ifndef JOYSTICK_H
#define JOYSTICK_H

class Joystick
{
  int xPin, yPin;
  int *zPin = NULL;
public:  
  Vector2<float> axes;
  bool isPressed = 0; // 1
  bool invertH = false; 
  bool invertV = false;

  Joystick(int pinX, int pinY, int* pinZ = NULL, bool invertHorizontal = false, bool invertVertical = false)
  {
    this->xPin = pinX;
    this->yPin = pinY;
    invertH = invertHorizontal;
    invertV = invertVertical;
    if (pinZ) {
      this->zPin = new int;
      *this->zPin = *pinZ;
      pinMode(*this->zPin, INPUT_PULLUP);
    }
  }
  ~Joystick() 
  {
    if (zPin) {
      delete zPin;
    }
  }
  void Start();
  Vector3<float> Read(bool normalize = false);
  void Print();
};

#endif
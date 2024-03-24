
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Vector.h>
#include <Joystick.h>

// QWIICMUX mux;
// JOYSTICK rawJoystick;
// int joystickCount = 0;

void Joystick::Start()
{
    
}

Vector3<float> Joystick::Read(bool normalize)
{
  static const int microADCResolution = 4095;
  static const int joystickADCResolution = 4095;//1023;
  static int halfResolution = joystickADCResolution / 2;
  static int rawMidpoint = halfResolution;
  static int xRawAbsErrorOffset = 320;
  static int yRawAbsErrorOffset = 340;

  int rawX = map(analogRead(this->xPin), 0, microADCResolution, 0, joystickADCResolution);
  int rawY = map(analogRead(this->yPin), 0, microADCResolution, 0, joystickADCResolution);
  if (zPin) {
    //Always inverted so that pressed = 1, else 0;
    isPressed = !digitalRead(*zPin);
  }  

  //------------ Fix reversed rawY axis to match down(-) up(+) standards for cartesian coords. --------------
  if (rawY != rawMidpoint)
  {
    rawY = joystickADCResolution - rawY;
  }

  // ==================================
  //            HORIZONTAL
  // ==================================
  if (rawX < (rawMidpoint - xRawAbsErrorOffset))
  {
    // Left
    axes.x = -map(rawX, 0, (rawMidpoint - xRawAbsErrorOffset), halfResolution, 0);
  }
  else if (rawX > (rawMidpoint + xRawAbsErrorOffset))
  {
    // Right
    axes.x = map(rawX, (rawMidpoint + xRawAbsErrorOffset), joystickADCResolution, 0, halfResolution);
  }
  else {
    axes.x = 0;
  }

  // ==================================
  //              VERTICAL
  // ==================================
  if (rawY < (rawMidpoint - yRawAbsErrorOffset))
  {
    // Down
    axes.y = -map(rawY, 0, (rawMidpoint - yRawAbsErrorOffset), halfResolution, 0);
  }
  else if (rawY > (rawMidpoint + yRawAbsErrorOffset))
  {
    // Up
    axes.y = map(rawY, (rawMidpoint + yRawAbsErrorOffset), joystickADCResolution, 0, halfResolution);
  }
  else {
    axes.y = 0;
  }
  
  // Invert axes if physically upside down
  if (invertH) {
    axes.x *= -1.0;
  }
  if (invertV)
  {
    axes.y *= -1.0;
  }

  if (normalize)
  {
    axes.Normalize();
  }

  Vector3<float> vec3 = Vector3<float>(axes.x, axes.y, isPressed);
  return vec3;
};

void Joystick::Print()
{
    Serial.println(String("X: ")+axes.x);
    Serial.println(String("Y: ")+axes.y);
    Serial.println(String("Pressed: ")+isPressed);
}
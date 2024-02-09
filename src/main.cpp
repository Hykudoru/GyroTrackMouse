#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 oled = Adafruit_SSD1306(128, 32, &Wire);
#include <BleKeyboard.h>
//BleKeyboard bleKeyboard;
#include <BleMouse.h>
BleMouse bleMouse;
#include <math.h>
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
// My libs
#include <Functions.h>
#include <Vector.h>
#include <MuxJoystick.h>
const int LEFT_JOYSTICK_MUX_PORT = 0;
const int RIGHT_JOYSTICK_MUX_PORT = 7;
extern int joystickCount;
MuxJoystick leftJoystick(LEFT_JOYSTICK_MUX_PORT);
MuxJoystick rightJoystick(RIGHT_JOYSTICK_MUX_PORT);
#include <Joystick.h>
#define SINGLE_JOYSTICK_MOUSE  

#define JOYSTICK_X_PIN 4
#define JOYSTICK_Y_PIN 3
#define JOYSTICK_Z_PIN 1
Joystick joystick = Joystick(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_Z_PIN, true, true);
#include <IMU.h>
IMU imu = IMU();
bool isMousePointer = false;

const float EPSILON = 0.00001;
bool EqZero(float val) 
{
  if (val < EPSILON && val > -EPSILON)
  {
    return true;
  }

  return false;
}

//pfunc can be reasigned at runtime to change the desired procedure invoked inside the default loop function.
typedef void (*pointerFunction)(void);
pointerFunction ptrMode;

#define DEBUGGING 1
#if defined(ESP32)
  const int BAUD_RATE = 115200;
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
  const int POT_SENSOR_PIN_GPIO_39 = 39;
#endif

const int MIN_MOUSE_MOVE_SPEED = 1;
const int MAX_MOUSE_MOVE_SPEED = 10;
const int MAX_MOUSE_SCROLL_SPEED = 1;
float moveAccel = 30; // reach max speed in 0.3s
float moveSpeed = MIN_MOUSE_MOVE_SPEED;
bool mouseActive = true;
bool righthanded = true;

float changeOfBasisMatrix_bs[3][3] = 
{
  {0.0, 0.0, 1.0},
  {-1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0}
};
// float changeOfBasisMatrix[3][3] = 
// {
//   {0.0, -1.0, 0.0},
//   {0.0, 0.0, -1.0},
//   {1.0, 0.0, 0.0}
// };
// Max RTC Memory = 8kb
RTC_DATA_ATTR int wakeCount = 0;
RTC_DATA_ATTR int sleepCount = 0;
RTC_DATA_ATTR bool sleeping = false;
unsigned long lastTimeTouched = 0;
unsigned long lastTimeIdle = millis(); 

// ================================================================================
//                                     SETUP       
// ================================================================================
void setup() 
{ 
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);

  // =========================
  //       SETUP DISPLAY
  // =========================
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.display();//displays initial adafruit image
  oled.clearDisplay();//clears initial adafruit image
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);

  // =========================
  //         SETUP IO
  // =========================
  // pinMode(BUTTON_A, INPUT_PULLUP);
  // pinMode(BUTTON_B, INPUT_PULLUP);
  // pinMode(BUTTON_C, INPUT_PULLUP);
  // touchAttachInterrupt(TOUCH_PIN_GPIO_33, TriggerWakeSleepState, 20);
  // esp_sleep_enable_touchpad_wakeup();
  leftJoystick.Start();
  rightJoystick.Start();
//=============================================================
// The dual joystick controller uses two sparkfun joysticks 
// oriented properly according to h and v axis marked on board.
// If using one joystick on the index finger then the joystick 
// is inverted AND then rotated. Later in the main loop the 
// x and y axis are swaped a certain way depending on if 
// left-handed or right-handed.
//=============================================================
  if (joystickCount < 2) {
    #if defined(INDEX_FINGER_JOYSTICK_MOUSE)
      leftJoystick.invertH = true;
      leftJoystick.invertV = true;
    #endif
  }

  bleMouse.begin();
  pixels.begin();
  pixels.setBrightness(20);

  oled.display();
  if(imu.Init())
  {
    isMousePointer = true;
    pixels.fill(0xFFFFFF);
    pixels.show();
    imu.Calibrate();
    pixels.fill(0x000000);
    pixels.show();
  }
 }

// =======================================================================================
//                                    MAIN PROGRAM                            
// =======================================================================================

unsigned long prevTime = 0;
float deltaTime = 0;
void loop() 
{
  deltaTime = ((float)(millis() - prevTime))/1000.0;
  prevTime = millis();
  
  // =========================
  //          MOUSE
  // =========================
  if (mouseActive && bleMouse.isConnected()) 
  { 
    Vector3<float> mouse = Vector3<float>(0,0,0);
    Vector3<float> mouse2 = Vector3<float>(0,0,0);
    Vector3<float> scroll = Vector3<float>(0,0,0);
    
    if (isMousePointer)
    {
      static Vector3<float> v0_s;
      imu.Update();
  
      Vector3<float> vf_s = Matrix3x3::Transpose(changeOfBasisMatrix_bs) * imu.rotationMatrix * changeOfBasisMatrix_bs * Vec3(0, 0, -1); // <0, 0, -1> = forward_s
      mouse = (vf_s-v0_s) * 3000;
      mouse.y *= -1; //API specific since up = -1 for some reason.
      v0_s = vf_s; 
      scroll = joystick.Read().Normalized();
      bleMouse.move(mouse.x, mouse.y/* -(mouse.z)*/, scroll.y, scroll.x);

      // JOYSTICK BUTTON PRESS
      static bool isPressing = false;
      if (scroll.z) 
      {
        if (!isPressing) 
        {
          isPressing = true;
          bleMouse.press(MOUSE_LEFT);
          pixels.fill(pixels.Color(rand(), rand(), rand()));
          pixels.show();
        }
        //delay(250); //since joystick clicks last longer than actual mouse 
      }
      else {
        if (isPressing) {
          isPressing = false; 
          bleMouse.release(MOUSE_LEFT);
          pixels.fill(0x000000);
          pixels.show();
        }
      }

      // COLOR
      float dot = DotProduct(vf_s, Vector3<float>(0,1,0));
      if (dot > 0) {
        pixels.fill(0x00ff00);
      } else {
        pixels.fill(0xff0000);
      }
      pixels.setBrightness(abs(dot) * 20);
      pixels.show();

      return;
    }


    // -------SINGLE JOYSTICK MOUSE---------
    #if defined(SINGLE_JOYSTICK_MOUSE)
      mouse = joystick.Read();
    #else
      mouse = leftJoystick.Read();
      // -------DUAL JOYSTICK MOUSE---------
      if (joystickCount > 1) {
        mouse2 = rightJoystick.Read();
      }
      // -------INDEX FINGER MOUSE---------
      #if defined(INDEX_FINGER_JOYSTICK_MOUSE)
        // Reverse x and y axis single handed index finger joystick
        Vector3<float> tmp = mouse;
        if (righthanded) {
          // RIGHT-HAND 
          mouse.x = -tmp.y;
          mouse.y = tmp.x;
        } else { 
          // LEFT-HAND
          mouse.x = tmp.y;
          mouse.y = -tmp.x;
        }
      #endif
    #endif
    
    // if either joystick moved
    if (mouse.Magnitude() > EPSILON || mouse2.Magnitude() > EPSILON)
    {
      mouse.Normalize();
      mouse2.Normalize();
      moveSpeed += moveAccel * deltaTime;
      mouse.x *= moveSpeed;
      mouse.y *= moveSpeed;
      mouse2.x *= moveSpeed;
      mouse2.y *= moveSpeed;

      // SCROLL: if both joysticks moving. Scroll direction depends on the resultant vector sum of the 2D axes.
      if (mouse.Magnitude() > EPSILON && mouse2.Magnitude() > EPSILON) 
      {
        scroll.x = constrain((mouse.x+mouse2.x), -MAX_MOUSE_SCROLL_SPEED, MAX_MOUSE_SCROLL_SPEED);
        scroll.y = constrain((mouse.y+mouse2.y), -MAX_MOUSE_SCROLL_SPEED, MAX_MOUSE_SCROLL_SPEED);
        
        bleMouse.move(0, 0, scroll.y, scroll.x);
      }
      // MOUSE MOVE
      else 
      {
        // Sum both vectors since we know only one joystick is moving while one or the other vector is zero.
        mouse.x = constrain(mouse.x, -MAX_MOUSE_MOVE_SPEED, MAX_MOUSE_MOVE_SPEED);
        mouse.y = constrain(mouse.y, -MAX_MOUSE_MOVE_SPEED, MAX_MOUSE_MOVE_SPEED);
        mouse2.x = constrain(mouse2.x, -MAX_MOUSE_MOVE_SPEED, MAX_MOUSE_MOVE_SPEED);
        mouse2.y = constrain(mouse2.y, -MAX_MOUSE_MOVE_SPEED, MAX_MOUSE_MOVE_SPEED);

        bleMouse.move((mouse.x+mouse2.x), -(mouse.y+mouse2.y));
      }

      lastTimeIdle = millis();
      mouse.Normalize();
      uint32_t color = pixels.Color(abs(mouse.x) * 255, 0, abs(mouse.y) * 255);
      pixels.fill(color);
      pixels.show();
    }
    else {
      moveSpeed = MIN_MOUSE_MOVE_SPEED;
      pixels.fill(0x000000);
      pixels.show();
    }

    // JOYSTICK BUTTON PRESS
    if (mouse.z || mouse2.z) 
    {
      bleMouse.click(MOUSE_LEFT);
      delay(250); //since joystick clicks last longer than actual mouse 
      lastTimeIdle = millis();
    }
  }

  delay(2);
}
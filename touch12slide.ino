/*******************************************************************************

 Bare Conductive MPR121 library
 ------------------------------
 
 SimpleTouch.ino - simple MPR121 touch detection demo with serial output
 
 Based on code by Jim Lindblom and plenty of inspiration from the Freescale 
 Semiconductor datasheets and application notes.
 
 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.
 
 This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 
 Unported License (CC BY-SA 3.0) http://creativecommons.org/licenses/by-sa/3.0/
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*******************************************************************************/

#include <MPR121.h>
#include <Wire.h>

#define numElectrodes 12
#define MAX_VALUE 24

int last = -1;
int curr = -1;
int number = 0;

int dataPin = 7;
int clockPin = 8;
int latchPin = 9;

enum states {LEVEL, VALUE};

struct Touch {
  char isTouched=0;
  unsigned long lastTouchTime=0;
  unsigned long lastReleaseTime=0;
  unsigned long lastEventTime=0;
  void touch(){
    this->isTouched=1;
    this->lastTouchTime=millis();
  }
  void release(){
    this->isTouched=0;
    this->lastReleaseTime=millis();
  }
};

struct Slider {
  unsigned long lastTime=0;
  unsigned long lastPeriod;
  struct Touch touchPin[12];
};

struct Slider slider;

int currentState = LEVEL;

/* reset func */
void (*pseudoReset)(void)=0;

void setup()
{
  Serial.begin(9600);
  while(!Serial);  // only needed if you want serial feedback with the
  		   // Arduino Leonardo or Bare Touch Board

  Wire.begin();
  
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  
  indicateStart();
  
  // 0x5C is the MPR121 I2C address on the Bare Touch Board
  if(!MPR121.begin(0x5A)){ 
    /*
    Serial.println("error setting up MPR121");  
    switch(MPR121.getError()){
      case NO_ERROR:
        Serial.println("no error");
        break;  
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;      
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
        break;      
    }
    */
    while(1);
  }
  
  // pin 4 is the MPR121 interrupt on the Bare Touch Board
  MPR121.setInterruptPin(2);
  // initial data update
  MPR121.updateTouchData();
}

void loop()
{
  if(MPR121.touchStatusChanged()){
    MPR121.updateTouchData();
    for(int i=0; i<numElectrodes; i++){
      static unsigned long lastTime = 0;
      static unsigned long lastRelease =0;
      static unsigned long lastTouch =0;
      static unsigned long lastPeriod = 0;
      if(MPR121.isNewTouch(i)){
        slider.touchPin[i].touch();
        lastTouch=millis();
      } else if(MPR121.isNewRelease(i)){
        curr = i;
        slider.touchPin[i].release();

        if((curr - last == -1)&&number>0){
          number--;
          vuPrint(number);
          if(millis()-lastTime < 100)
          for(int j=0; j<2; j++){
            if(number>0){
              number--;
              vuPrint(number);
            }
          }
          Serial.println(number);
        }
        else if((last - curr == -1)&&number<MAX_VALUE){
          number++;
          vuPrint(number);
            if(millis()-lastTime < 100)
            for(int j=0; j<2; j++){
              if(number<MAX_VALUE){
                number++;
                vuPrint(number);
              }
            }
          Serial.println(number);
        }
        
        lastRelease=lastTime=millis();
        last = curr;
      }
    }
  }
  for(int i=0; i<numElectrodes; i++){
    if(slider.touchPin[i].isTouched && (millis()-slider.touchPin[i].lastTouchTime>1000) && (millis()-slider.touchPin[i].lastEventTime>1000)){
      slider.touchPin[i].lastEventTime=millis();
      changeState();
      number=(i+1)<<1;
      vuPrint(number);
     }
     else if(slider.touchPin[i].isTouched && (millis()-slider.touchPin[i].lastTouchTime>5000)){
       pseudoReset();
     }
  }
}

void changeState(){
  if(currentState == LEVEL)
    currentState=VALUE;
  else if(currentState == VALUE)
    currentState=LEVEL;
}

void vuPrint(int vuValue){
  if(currentState == LEVEL)
    vuPrintLevel(vuValue);
  else if(currentState == VALUE)
    vuPrintValue(vuValue);
}

void vuPrintValue(int vuValue){
  int segment[3]={0, 0, 0};
  if(vuValue<0 || vuValue>MAX_VALUE)
    return;
  if(vuValue==0){
  }
  else if(vuValue <= 8){
    segment[0] = (1<<vuValue-1);
  }
  else if(vuValue <= 16){
    segment[1]=(1<<(vuValue-9));
  }
  else if(vuValue <= 24){
   segment[2]=(1<<(vuValue-17));
  }
  digitalWrite(latchPin, LOW);
  for(int i=0; i<3; i++){
    shiftOut(dataPin, clockPin, LSBFIRST, segment[i]);
  }
  digitalWrite(latchPin, HIGH);
}

void vuPrintLevel(int vuValue){
  int segment[3]={0, 0, 0};
  if(vuValue<0 || vuValue>MAX_VALUE)
    return;
  if(vuValue <= 8){
    segment[0] = (1<<vuValue)-1;
  }
  else if(vuValue <= 16){
    segment[0]=0xff;
    segment[1]=(1<<(vuValue-8))-1;
  }
  else if(vuValue <= 24){
   segment[0]=0xff;
   segment[1]=0xff;
   segment[2]=(1<<(vuValue-16))-1;
  }
  digitalWrite(latchPin, LOW);
  for(int i=0; i<3; i++){
    shiftOut(dataPin, clockPin, LSBFIRST, segment[i]);
  }
  digitalWrite(latchPin, HIGH);
}

/* startup indicator */
void indicateStart(){
  for(int i=0; i<3; i++){
    vuPrint(0);
    delay(200);
    vuPrint(MAX_VALUE);
    delay(200);
  }
  vuPrint(0);
}

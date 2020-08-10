//based of code by Tyler Glaiel
//for this mask: https://twitter.com/TylerGlaiel/status/1265035386109128704
//modified for Nano BLE Sense by Chris Azer

//You will need to also include these libraries in the Arduino IDE
//see here: https://www.arduino.cc/en/guide/libraries
#include <ArduinoBLE.h>
#include <Arduino_APDS9960.h>
#include <Arduino_LPS22HB.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <gamma.h>
#include <PDM.h>
#include <arm_math.h>
#include <Adafruit_LEDBackpack.h>


#define MICROPHONE_BUFFER_SIZE_IN_WORDS (256U)
#define MICROPHONE_BUFFER_SIZE_IN_BYTES (MICROPHONE_BUFFER_SIZE_IN_WORDS * sizeof(int16_t))
#define lengthof(A) ((sizeof((A))/sizeof((A)[0])))

/* MP34DT05 Microphone data buffer with a bit depth of 16. Also a variable for the RMS value */
int16_t microphoneBuffer[MICROPHONE_BUFFER_SIZE_IN_WORDS];
int16_t microphoneRMSValue;

/* Used as a simple flag to know when microphone buffer is full and RMS value
 * can be computed. */
bool microphoneBufferReadyFlag;

const PROGMEM uint8_t mouth_0[] = {
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

const PROGMEM uint8_t mouth_4[] = {
    0,0,1,1,1,1,0,0,
    0,1,0,0,0,0,1,0,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    0,1,0,0,0,0,1,0,
    0,0,1,1,1,1,0,0
};

const PROGMEM uint8_t mouth_3[] = {
    0,0,0,0,0,0,0,0,
    0,0,1,1,1,1,0,0,
    0,1,0,0,0,0,1,0,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    0,1,0,0,0,0,1,0,
    0,0,1,1,1,1,0,0,
    0,0,0,0,0,0,0,0
};

const PROGMEM uint8_t mouth_2[] = {
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,1,1,1,1,1,1,0,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    0,1,1,1,1,1,1,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

const PROGMEM uint8_t mouth_1[] = {
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,1,1,0,0,0,
    1,1,1,0,0,1,1,1,
    1,1,1,0,0,1,1,1,
    0,0,0,1,1,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

const PROGMEM uint8_t mouth_smile[] = {
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    1,0,0,0,0,0,0,1,
    1,1,0,0,0,0,1,1,
    0,1,1,1,1,1,1,0,
    0,0,2,2,2,2,0,0,
    0,0,2,2,2,2,0,0,
    0,0,0,2,2,0,0,0
};

uint16_t palette[8] = {};
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 6,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS    + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

void setup() {
  matrix.begin();
  palette[0] = matrix.Color(0,0,0);
  palette[1] = matrix.Color(0,255,0);
  palette[2] = matrix.Color(250,128,114);
  palette[3] = matrix.Color(255,0,0);
  palette[4] = matrix.Color(0,255,255);
  palette[5] = matrix.Color(255,0,255);
  palette[6] = matrix.Color(255,255,0);
  palette[7] = matrix.Color(255,255,255);

 Serial.begin(9600);

 /* Initialise microphone buffer ready flag */
 microphoneBufferReadyFlag = false;

 /* PDM setup for MP34DT05 microphone */
 /* configure the data receive callback to transfer data to local buffer */
 PDM.onReceive(Microphone_availablePDMDataCallback);
 
 if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM!");
    while (1);
 }

 /* Adjust microphone sensitivity */
 PDM.setGain(5);

}

void Microphone_availablePDMDataCallback()
{
 // query the number of bytes available
 int bytesAvailable = PDM.available();

 if(bytesAvailable == MICROPHONE_BUFFER_SIZE_IN_BYTES)
 {
   PDM.read(microphoneBuffer, bytesAvailable);
   microphoneBufferReadyFlag = true;
 }
}

/* For future use. Can display scrolling text */
void scrollText(String textToDisplay) {
  int x = matrix.width();

  // Account for 6 pixel wide characters plus a space
  int pixelsInText = textToDisplay.length() * 7;

  matrix.setCursor(x, 0);
  matrix.print(textToDisplay);
  matrix.show();

  while(x > (matrix.width() - pixelsInText)) {
    matrix.fillScreen(matrix.Color(255,0,0));
    matrix.setCursor(--x, 0);
    matrix.print(textToDisplay);
    matrix.show();
    delay(150);
  }
}

void Microphone_computeRMSValue(void)
{
 arm_rms_q15((q15_t*)microphoneBuffer, MICROPHONE_BUFFER_SIZE_IN_WORDS, (q15_t*)&microphoneRMSValue);
}

void drawImage(uint8_t image[]){
    int bufferindex = 0;
    for (int y = 7 ; y >= 0 ; y--){ //after the color array is built, they are pushed into this loop where it build the each pixel 
      for (int x = 0 ; x < 8 ; x++){     //and displays according to the color array
        matrix.drawPixel(x,y,palette[image[bufferindex]]);
        bufferindex++; 
      }
    }
    matrix.show();
}

int pop_detection = 0;
bool smiling = false;
unsigned long smiletimer = 0;
unsigned long last_face = 0;

float vol = 0;
const uint16_t samples = 128;

void loop() {
    float nvol = 0;
    
    /* If the microphone buffer is full, compute the RMS value */
    if(microphoneBufferReadyFlag)
    {
       Microphone_computeRMSValue();
       nvol = max(microphoneRMSValue,nvol);
       microphoneBufferReadyFlag = false;
    }

    vol = (nvol + 1.0*vol)/2.0;

    if(nvol > 200){
        pop_detection += 1;
        if(pop_detection > 2) {
            smiling = false;
            last_face = millis();
        }
    } else {
        if(pop_detection > 0 && pop_detection <= 2) {
            if(millis() > last_face + 1000){
                smiling = true;
                smiletimer = millis() + 2000;
            }
        }
        pop_detection = 0;
    }

    if(millis() > smiletimer) smiling = false;

    Serial.println(nvol);
    if(smiling){
        drawImage(mouth_smile);
    } else if(vol < 200){
        drawImage(mouth_0);
    } else if(vol < 250){
        drawImage(mouth_1);
    } else if(vol < 350){
        drawImage(mouth_2);
    } else if(vol < 450){
        drawImage(mouth_3);
    } else {
        drawImage(mouth_4);
    }
    
   /* Delay between buffer reads. */
   delay(13);
}

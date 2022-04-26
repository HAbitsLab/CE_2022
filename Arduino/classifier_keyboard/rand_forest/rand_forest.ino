#include "RandomForest.h"
#include <PluggableUSBHID.h>
#include <USBKeyboard.h>

#define MACOS 

// this class will be different if you used another type of classifier, just check the model.h file
Eloquent::ML::Port::RandomForest classifier;


// use table: https://apps.timwhitlock.info/emoji/tables/unicode
const int bicep = 0x1f4aa;
const int punch = 0x1f44a;



void classify(float* x_sample) {
  
    Serial.print("Predicted class: ");
    if (classifier.predict(x_sample) == 0)
       sentUtf8(bicep);
    else
      sentUtf8(punch);
    //Serial.println(classifier.predictLabel(x_sample));
}




USBKeyboard keyboard;


/*
  IMU Classifier

  This example uses the on-board IMU to start reading acceleration and gyroscope
  data from on-board IMU, once enough samples are read, it then uses a
  TensorFlow Lite (Micro) model to try to classify the movement as a known gesture.

  Note: The direct use of C/C++ pointers, namespaces, and dynamic memory is generally
        discouraged in Arduino examples, and in the future the TensorFlowLite library
        might change to make the sketch simpler.

  The circuit:
  - Arduino Nano 33 BLE or Arduino Nano 33 BLE Sense board.

  Created by Don Coleman, Sandeep Mistry
  Modified by Dominic Pajak, Sandeep Mistry

  This example code is in the public domain.
*/

#include <Arduino_LSM9DS1.h>


const float accelerationThreshold = 2.5; // threshold of significant in G's
const int numSamples = 119;

int samplesRead = numSamples;

const uint8_t axes = 3;
float accel[numSamples][axes];
float gyro[numSamples][axes];


// array to map gesture index to a name
const char* GESTURES[] = {
  "punch",
  "flex"
};

#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // print out the samples rates of the IMUs
  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  Serial.println();

}

void loop() {
  float aX, aY, aZ, gX, gY, gZ;

  // wait for significant motion
  while (samplesRead == numSamples) {
    if (IMU.accelerationAvailable()) {
      // read the acceleration data
      IMU.readAcceleration(aX, aY, aZ);

      // sum up the absolutes
      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      // check if it's above the threshold
      if (aSum >= accelerationThreshold) {
        // reset the sample read count
        samplesRead = 0;
        break;
      }
    }
  }

  // check if the all the required samples have been read since
  // the last time the significant motion was detected
  while (samplesRead < numSamples) {
    // check if new acceleration AND gyroscope data is available
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // read the acceleration and gyroscope data
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      accel[samplesRead][0] = aX;
      accel[samplesRead][1] = aY;
      accel[samplesRead][2] = aZ;

      gyro[samplesRead][0] = gX;
      gyro[samplesRead][1] = gY;
      gyro[samplesRead][2] = gZ;

      // normalize the IMU data between 0 to 1 and store in the model's
      // input tensor
      
      samplesRead++;

      if (samplesRead == numSamples) {

        
        // Run inferencing
        // Loop through the output tensor values from the model
        float max_aX = 0;
        float max_aY = 0;
        float max_aZ = 0;
        float max_gX = 0;
        float max_gY = 0;
        float max_gZ = 0;

        
        for (int i = 0; i < numSamples; i++) {
          if (gyro[i][0] > max_gX)
            max_gX = gyro[i][0];

          if (gyro[i][1] > max_gY)
            max_gY = gyro[i][1];

          if (gyro[i][2] > max_gZ)
            max_gZ = gyro[i][2];

          if (accel[i][0] > max_aX)
            max_aX = accel[i][0];
          
          if (accel[i][1] > max_aY)
            max_aY = accel[i][1];
          
          if (accel[i][2] > max_aZ)
            max_aZ = accel[i][2];
         
        }
        float input[6] = {max_aX, max_aY, max_aZ, max_gX, max_gY, max_gZ};
        classify(input);
        Serial.print(max_aX);
        Serial.println();
      }
    }
  }
}


void sentUtf8(unsigned long c) {
  String s;

#if defined(MACOS)
  // https://apple.stackexchange.com/questions/183045/how-can-i-type-unicode-characters-without-using-the-mouse

  s = String(utf8ToUtf16(c), HEX);

  for (int i = 0; i < s.length(); i++) {
    keyboard.key_code(s[i], KEY_ALT);
  }
#elif defined(LINUX)
  s = String(c, HEX);

  keyboard.key_code('u', KEY_CTRL | KEY_SHIFT);

  for (int i = 0; i < s.length(); i++) {
    keyboard.key_code(s[i]);
  }
#endif
  keyboard.key_code(' ');
}

// based on https://stackoverflow.com/a/6240819/2020087
unsigned long utf8ToUtf16(unsigned long in) {
  unsigned long result;

  in -= 0x10000;

  result |= (in & 0x3ff);
  result |= (in << 6) & 0x03ff0000;
  result |= 0xd800dc00;

  return result;
}

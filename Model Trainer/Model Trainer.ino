
#include <Wire.h>
//#include <WiFi.h>
const int MPU_addr = 0x68; // I2C address of the MPU-6050
float aX, aY, aZ, gX, gY, gZ, Tmp;
const float accelerationThreshold = 2.0; // threshold of significant in G's
const int numSamples = 119;
int samplesRead = numSamples;
// WiFi network info.
const char *ssid = "Orange-2176";     // Enter your WiFi Name
const char *pass =  "3FA62F5C"; // Enter your WiFi Password
//WiFiServer server(80);
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  Serial.println("aX,aY,aZ,gX,gY,gZ");
  //WiFi.begin(ssid, pass);
 // while (WiFi.status() != WL_CONNECTED)
  //{
   // delay(500);

  //}

  //server.begin();

}
void loop() {

  while (samplesRead == numSamples) {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
    aX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
    if (aX > 32768) {
      aX = aX - 65536;
    }
    aY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    if (aY > 32768) {
      aY = aY - 65536;
    }
    aZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    if (aZ > 32768) {
      aZ = aZ - 65536;
    }
    //Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    gX = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    gY = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    gZ = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
    float aSum = fabs(aX) / 16384.0 + fabs(aY) / 16384.0 + fabs(aZ) / 16384.0;

    // check if it's above the threshold
    if (aSum >= accelerationThreshold) {
      // reset the sample read count
      samplesRead = 0;
      break;
    }
  }
  // check if the all the required samples have been read since
  // the last time the significant motion was detected
  while (samplesRead < numSamples) {
    // check if both new acceleration and gyroscope data is
    // available
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
    aX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
    if (aX > 32768) {
      aX = aX - 65536;
    }
    aY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    if (aY > 32768) {
      aY = aY - 65536;
    }
    aZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    if (aZ > 32768) {
      aZ = aZ - 65536;
    }
    //Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    gX = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
     if (gX > 32768) {
      gX = gX - 65536;
    }
    gY = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
     if (gY > 32768) {
      gY = gY - 65536;
    }
    gZ = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
     if (gZ > 32768) {
      gZ = gZ - 65536;
    }
    samplesRead++;
    delay(1);
    // print the data in CSV format
    Serial.print(aX / 16384.0, 3);
    Serial.print(',');
    Serial.print(aY / 16384.0, 3);
    Serial.print(',');
    Serial.print(aZ / 16384.0, 3);
    Serial.print(',');
    Serial.print(gX/131.0, 3);
    Serial.print(',');
    Serial.print(gY/131.0, 3);
    Serial.print(',');
    Serial.print(gZ/131.0, 3);
    Serial.println();

    if (samplesRead == numSamples) {
      // add an empty line if it's the last sample
      Serial.println();

    }

  }
}

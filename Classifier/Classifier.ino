#include <ArduinoJson.h>
#include <TensorFlowLite_ESP32.h>
#include <ListLib.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#include <Wire.h>
#include <WiFi.h>
#include "Esp32MQTTClient.h"
#include <tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h>
#include <tensorflow/lite/experimental/micro/micro_error_reporter.h>
#include <tensorflow/lite/experimental/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>
#include "model.h"

// -------------------------Constant definition--------------
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)
#define INTERVAL 10000
#define MESSAGE_MAX_LEN 256
const int MPU_addr = 0x68;
//---------------------------------Wifi && IoTHub declarations
// Please input the SSID and password of WiFi
const char* ssid     = "MIWIFI_5G_TxXa";
const char* password = "hXQUXnbU";
const float accelerationThreshold = 2.5; // threshold of significant in G's
int timeout = 0;
const char *messageData = "{\"messageId\":%d, \"Temperature\":%f, \"Humidity\":%f}";

static const char* connectionString = "HostName=iothubpesas.azure-devices.net;DeviceId=DispositivoTest1;SharedAccessKey=bkQmrzz3HoeRp29GNT792jpX7aK8tjuEoIDjsJKZiy0=";
static bool hasIoTHub = false;
static bool hasWifi = false;
int messageCount = 1;
static bool messageSending = true;
static uint64_t send_interval_ms;

//---------------------PN532 Global Declarations
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
uint8_t user[4] = {0xFF, 0xFF, 0xFF, 0xFF};
uint8_t defaultUser[4] = {0xFF, 0xFF, 0xFF, 0xFF};
List<uint8_t> listPesas;
int weight = 6;

// -----------global variables used for TensorFlow Lite (Micro)
//PN532 Global Declarations
tflite::MicroErrorReporter tflErrorReporter;
// pull in all the TFLM ops, you can remove this line and
// only pull in the TFLM ops you need, if would like to reduce
// the compiled size of the sketch.
tflite::ops::micro::AllOpsResolver tflOpsResolver;
const int numSamples = 119;
int samplesRead = numSamples;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

char* GESTURES[] = {
  "Curl_Biceps",
  "SquatQAw"
  
};
#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))
int GESTURES_PROB[NUM_GESTURES];
// Create a static memory buffer for TFLM, the size may need to
// be adjusted based on the model you are using
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];
int aas= 0;
int repeticiones = 0;
//------------------Send data Json declarations
StaticJsonDocument<2500> docu;
JsonArray serie = docu.createNestedArray("repeticiones");

// array to map gesture index to a name


void Task1code( void * pvParameters ){
   float aX, aY, aZ, gX, gY, gZ;
   
    {for(;;){
      if(timeout > 2)
        {
          
          memcpy(user,defaultUser,sizeof(defaultUser));
          timeout = 0;
         }
      while (isEqualArray(user,defaultUser,4))
      {
        delay(10);
      }
    while (samplesRead == numSamples && aas < 500) {
    
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
      repeticiones = repeticiones + 1;
      String jason;
      StaticJsonDocument<300> reps;
      reps["time"] = millis();
      reps["idRep"] = repeticiones;
      serie.add(reps);
      aas=0;
      serializeJson(serie, jason);
      Serial.print("Número de repeticiones: ");
      Serial.println(repeticiones);      
    }
    aas = aas + 1;
    delay(10);
  }
   //SerializeObject(serie);
  while (samplesRead < numSamples && aas < 500) {
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
    // normalize the IMU data between 0 to 1 and store in the model's
      // input tensor
      tflInputTensor->data.f[samplesRead * 6 + 0] = (aX/16384 + 4.0) / 8.0;
      tflInputTensor->data.f[samplesRead * 6 + 1] = (aY/16384 + 4.0) / 8.0;
      tflInputTensor->data.f[samplesRead * 6 + 2] = (aZ/16384 + 4.0) / 8.0;
      tflInputTensor->data.f[samplesRead * 6 + 3] = (gX/131.0 + 360.0) / 720.0;
      tflInputTensor->data.f[samplesRead * 6 + 4] = (gY/131.0 + 360.0) / 720.0;
      tflInputTensor->data.f[samplesRead * 6 + 5] = (gZ/131.0 + 360.0) / 720.0;      
      
      samplesRead++;


      if (samplesRead == numSamples) {
        // Run inferencing
        TfLiteStatus invokeStatus = tflInterpreter->Invoke();
        if (invokeStatus != kTfLiteOk) {
          Serial.println("Invoke failed!");
          while (1);
          return;
        }
        Serial.println("Prección realizada: ");
        // Loop through the output tensor values from the model        
        for (int i = 0; i < NUM_GESTURES; i++) {
          if (tflOutputTensor->data.f[i] >= 0.65)
          {
            GESTURES_PROB[i]++;  
          }
          Serial.print(GESTURES[i]);
          Serial.print(": ");
          Serial.println(tflOutputTensor->data.f[i], 6);
        }
        
      }
      
    }
    delay(500);
    ///Envia los datos/////////////
    
    
    
    if (hasWifi && hasIoTHub && aas >= 500 && repeticiones != 0)
    {
    if ((int)(millis() - send_interval_ms) >= INTERVAL)
    {
      // Send teperature data
      char messagePayload[MESSAGE_MAX_LEN];
      int Weigth = weight;
      char* typeas = GetExerciseType(GESTURES_PROB);
      String Type(typeas);
      String UserId(user[1],HEX);
      String s = SerializeObject(UserId,Weigth,Type,serie);
      const char *c = s.c_str();
      //snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, messageCount++, temperature, humidity);
      
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(c, MESSAGE);
      Esp32MQTTClient_SendEventInstance(message);
      send_interval_ms = millis();
//      SerializeObject(serie);
    }
    else
    {
      Esp32MQTTClient_Check();
    }
    docu.clear();
    repeticiones = 0;
    memset(GESTURES_PROB,0,sizeof(GESTURES_PROB));
    //SerializeObject(serie);
    serie = docu.createNestedArray("repeticiones");
    aas=0;
    delay(10);
  }
  if(aas == 500){
    aas =0;
  timeout++;
  }
    }
}
}

String  SerializeObject(String userId, int weigth, String type, JsonArray reps)
{
    String json;
    StaticJsonDocument<3000> doc;
    doc["userId"] = userId;
    doc["weigth"] = weigth;
    doc["type"] = type;
    doc["repeticiones"] = reps;
    serializeJson(doc, json);
    Serial.println(json);
    return json;
}
bool isEqualArray(uint8_t* ArrayA, uint8_t* ArrayB,int largaria){
  
  bool isEqual = true;
  for (uint8_t i = 0; i < largaria; i++){
    
    
      if (ArrayA[i] != ArrayB[i]) isEqual = false;
    
  }
  return isEqual;
}
char* GetExerciseType(int* probabilities)
 {
  float maxvalue=0;
  int kmax=0;
  for (int k=0; k<NUM_GESTURES; k++){
    if (probabilities[k] > maxvalue) {
      maxvalue = probabilities[k];
      kmax = k;
    }
  }
  return GESTURES[kmax];
 }
static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
  }
TaskHandle_t Task1;




void setup() {
  Serial.begin(115200);
  Serial.println(xPortGetCoreID());
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true); 
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  Serial.print(versiondata);
  while  (! versiondata) {
    Serial.print("Didn't find PN53x board");
    versiondata = nfc.getFirmwareVersion();
    //while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A Card ...");
  delay(10);
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
    hasWifi = false;
  }
  hasWifi = true;
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(" > IoT Hub");
  if (!Esp32MQTTClient_Init((const uint8_t*)connectionString, true))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
    return;
    }
  hasIoTHub = true;
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
  Serial.println("Start sending events.");
  randomSeed(analogRead(0));
  send_interval_ms = millis();
  Serial.println();

  // get the TFL representation of the model byte array
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }

  // Create an interpreter to run the model
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  // Allocate memory for the model's input and output tensors
  tflInterpreter->AllocateTensors();

  // Get pointers for the model's input and output tensors
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
  //SerializeObject();
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */  
  delay(500); 
}

void loop() {
 
  ////Regoge los datos/////////////////////
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)  
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
   if (success) 
   {
    // Display some basic information about the card
   //nfc.PrintHex(uid, uidLength);
    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ... 
    
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      //Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    
      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
      if (success)
      {
        uint8_t data[16];
        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);
        if (success)
        {
          uint8_t pesa1[7];
          // Data seems to have been read ... spit it out
         
          //nfc.PrintHexChar(data, 16);
          memcpy(pesa1, uid, sizeof(uid[0])*7);
          
          int block4 = (data[0])*10 + data[1] ;
          
          
          
          
          
          
          if ( block4 == 0 ){
            memcpy(user, uid, sizeof(uid[0])*7);
            Serial.print("Usuario identificado: ");Serial.println(uid[1],HEX);
          }else if (listPesas.Contains(pesa1[0])){
             int index = listPesas.IndexOf(pesa1[0]);
             listPesas.Remove(index);
             listPesas.Trim();
             weight = weight - block4;
             Serial.print("Peso retirado: ");Serial.print(block4);Serial.println("kg");
          }else 
          {
            Serial.print("Peso introducido: ");Serial.print(block4);Serial.println("kg");
            memcpy(pesa1, uid, sizeof(uid));
            listPesas.Insert(pesa1[0]);
            weight = weight + block4;
          }
           Serial.print("Actualmente el peso es: "); Serial.print(weight); Serial.println("kg");         
        
        }  
        
        }
      }
      delay(1000);
    }
    
}
   
  

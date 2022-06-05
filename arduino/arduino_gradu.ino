#include <ArduinoBLE.h>

#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

#include "model.h"

tflite::MicroErrorReporter tflErrorReporter;

tflite::AllOpsResolver tflOpsResolver;

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

constexpr int tensorArenaSize = 8 * 2949;
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));

const char* FEATURES[] = {
  "dewpoint",
  "ground_temp",
  "slope"
};

int characteristicWritten = 0;
float dewpoint;
float temp;
float slope;
float inference;

#define NUM_FEATURES (sizeof(FEATUERS) / sizeof(FEATURES[0]))

const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1213";
const char* temperatureServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";
const char* dewpointServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1215";
const char* slopeServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1216";
const char* inferenceServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1217";

BLEService* slipperyService = nullptr;
BLEFloatCharacteristic* rxTemperatureSlipperyCharacteristic = nullptr;
BLEFloatCharacteristic* rxDewpointSlipperyServiceCharacteristic = nullptr;
BLEFloatCharacteristic* rxSlopeSlipperyServiceCharacteristic = nullptr;
BLEFloatCharacteristic* txInferenceSlipperyServiceCharacteristic = nullptr;

void setup() {
  // put your setup code here, to run once:

  slipperyService = new BLEService(deviceServiceUuid);
  rxTemperatureSlipperyCharacteristic = new BLEFloatCharacteristic(temperatureServiceCharacteristicUuid, BLERead | BLEWrite | BLEWriteWithoutResponse);
  rxDewpointSlipperyServiceCharacteristic = new BLEFloatCharacteristic(dewpointServiceCharacteristicUuid, BLERead | BLEWrite | BLEWriteWithoutResponse);
  rxSlopeSlipperyServiceCharacteristic = new BLEFloatCharacteristic(slopeServiceCharacteristicUuid, BLERead | BLEWrite | BLEWriteWithoutResponse);
  txInferenceSlipperyServiceCharacteristic = new BLEFloatCharacteristic(inferenceServiceCharacteristicUuid, BLERead | BLEWrite | BLEWriteWithoutResponse);
  
  Serial.begin(9600);
  while(!Serial);

  Serial.println("+++ Initializing TFLite model +++");
  tflModel = tflite::GetModel(model);
  if(tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while(1);
  }
  Serial.println("+++ Success! +++");

  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  tflInterpreter->AllocateTensors();
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);

  if(!BLE.begin()) {
    Serial.println("- Starting Bluetooth Low Energy module failed!");
    while(1);
  }
  //txInferenceSlipperyServiceCharacteristic.setEventHandler
  
  BLE.setAdvertisedService(*slipperyService);
  slipperyService->addCharacteristic(*rxTemperatureSlipperyCharacteristic);
  slipperyService->addCharacteristic(*rxDewpointSlipperyServiceCharacteristic);
  slipperyService->addCharacteristic(*rxSlopeSlipperyServiceCharacteristic);
  slipperyService->addCharacteristic(*txInferenceSlipperyServiceCharacteristic);
  BLE.addService(*slipperyService);
  BLE.setLocalName("Arduino Nano 33 BLE Sense (Peripheral)");

  BLE.setEventHandler(BLEConnected, onBLEConnected);
  BLE.setEventHandler(BLEDisconnected, onBLEDisconnected);
  
  rxDewpointSlipperyServiceCharacteristic->setEventHandler(BLEWritten, onDewpointCharacteristicWrite);
  rxTemperatureSlipperyCharacteristic->setEventHandler(BLEWritten, onTempCharacteristicWrite);
  rxSlopeSlipperyServiceCharacteristic->setEventHandler(BLEWritten, onSlopeCharacteristicWrite);
  
  BLE.advertise();

  Serial.println("Peripheral advertising info: ");
  Serial.print("MAC: ");
  Serial.println(BLE.address());
  Serial.print("Service UUID: ");
  Serial.println(slipperyService->uuid());
  Serial.println();
  Serial.println("+++ Discovering central device... +++");
}

void loop() {
  // put your main code here, to run repeatedly:


  BLEDevice central = BLE.central();
  delay(1);
}

void onDewpointCharacteristicWrite(BLEDevice central, BLECharacteristic characteristic) {
  Serial.print("Write event to dewpoint char with value ");
  Serial.println(rxDewpointSlipperyServiceCharacteristic->value());
  dewpoint = rxDewpointSlipperyServiceCharacteristic->value();
  tflInputTensor->data.f[0] = dewpoint;
}

void onTempCharacteristicWrite(BLEDevice central, BLECharacteristic characteristic) {
  Serial.print("Write event to temperature char with value ");
  Serial.println(rxTemperatureSlipperyCharacteristic->value());
  dewpoint = rxTemperatureSlipperyCharacteristic->value();
  tflInputTensor->data.f[1] = temp;  
}

void onSlopeCharacteristicWrite(BLEDevice central, BLECharacteristic characteristic) {
  Serial.print("Write event to slope char with value ");
  Serial.println(rxSlopeSlipperyServiceCharacteristic->value());
  dewpoint = rxSlopeSlipperyServiceCharacteristic->value();
  tflInputTensor->data.f[2] = slope;
  
  makeInference();
}

void makeInference() {
    Serial.println("Performing inference");
    
    inference = tflOutputTensor->data.f[0];
    
    Serial.print("Writing value: ");
    Serial.print(inference);
    Serial.println(" to inference characteristic...");
    txInferenceSlipperyServiceCharacteristic->writeValue(inference);
}

void onBLEConnected(BLEDevice central) {
    Serial.println("* Connected to central device!");
    Serial.print("* Device MAC address: ");
    Serial.println(central.address());
}

void onBLEDisconnected(BLEDevice central) {
  Serial.println("* Disconnected from central device!");
  Serial.print("* Device MAC address: ");
  Serial.println(central.address());
}

#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Base64.h>  // Base64 library

SoftwareSerial mySerial(2, 3);  // RX, TX for fingerprint sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(9600);  // Communication with Python
  finger.begin(57600);  // Communication with fingerprint sensor

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found.");
  } else {
    Serial.println("Fingerprint sensor not found.");
    while (1);  // Halt if sensor is not found
  }
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command == "SCAN_FINGER") {
      Serial.println("Starting fingerprint scan...");
      sendFingerprintTemplate();
    }
  }
  delay(100);  // Small delay
}

void sendFingerprintTemplate() {
  while (true) {
    int result = finger.getImage();
    if (result == FINGERPRINT_OK) {
      Serial.println("Fingerprint found successfully!");

      if (finger.image2Tz(1) != FINGERPRINT_OK) {
        Serial.println("Failed to convert image to template.");
        return;
      }

      delay(2000);  // Wait for the second scan

      Serial.println("Place finger on sensor again...");
      while (finger.getImage() != FINGERPRINT_OK) {
        delay(600);
      }

      if (finger.image2Tz(2) != FINGERPRINT_OK) {
        Serial.println("Failed to convert second image.");
        return;
      }

      if (finger.createModel() != FINGERPRINT_OK) {
        Serial.println("Failed to create fingerprint model.");
        delay(2000);
        continue;  // Retry
      }

      if (finger.getModel() == FINGERPRINT_OK) {
        Serial.println("BEGIN");

        // Sending model data in chunks
        uint8_t model[534]; // 534 bytes is typical for template size
        finger.getModel();  // Prepare data to be read
        for (int i = 0; i < 534; i++) {
          model[i] = mySerial.read();  // Collect raw bytes
        }

        // Base64 encode full data
        char encodedData[Base64.encodedLength(534)];
        Base64.encode(encodedData, (char *)model, 534);

        Serial.write(encodedData);  // Send all encoded bytes
        delay(1500);
        Serial.println("FINISH");
      } else {
        Serial.println("Failed to retrieve fingerprint model data.");
        sendFingerprintTemplate();  // Retry
      }
      return;
    } else {
      Serial.println("No finger detected. Retrying...");
      delay(500);
    }
  }
}











// PRACTICE CODE TO MAKE SURE THE SCANNER WORKS ========================
// #include <Adafruit_Fingerprint.h>
// #include <SoftwareSerial.h>

// // Define RX and TX pins for the fingerprint sensor
// SoftwareSerial mySerial(2, 3);  // RX, TX
// Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// void setup() {
//   Serial.begin(9600);  // Start serial communication with Python
//   finger.begin(57600); // Start serial communication with the fingerprint sensor

//   if (finger.verifyPassword()) {
//     Serial.println("Fingerprint sensor found.");
//   } else {
//     Serial.println("Fingerprint sensor not found.");
//     while (1);  // Halt if sensor is not found
//   }
// }

// void loop() {
//   // Continuously check for a fingerprint
//   int result = finger.getImage();

//   if (result == FINGERPRINT_OK) {
//     Serial.println("Fingerprint detected!");
//     // Proceed to convert the image to a template and create a model
//     if (finger.image2Tz() != FINGERPRINT_OK) {
//       Serial.println("Could not convert image.");
//       return;
//     }
//     // Attempt to find a match
//     if (finger.fingerFastSearch() != FINGERPRINT_OK) {
//       Serial.println("No match found.");
//     } else {
//       Serial.print("Found ID #"); 
//       Serial.println(finger.fingerID);
//       // Here you can add code to send the fingerprint ID back to Python if needed
//     }
//   } else {
//     Serial.println("No finger detected.");
//   }
//   delay(500);  // Small delay to avoid flooding the Serial output
// }






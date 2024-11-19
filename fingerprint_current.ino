// bool isProcessing = false;

// void loop() {
//   if (Serial.available() > 0 && !isProcessing) {
//     isProcessing = true;
//     String command = Serial.readStringUntil('\n');
//     command.trim();
    
//     if (command == "SCAN_FINGER") {
//       sendFingerprintTemplate();
//     } else if (command == "VERIFY_FINGER") {
//       verify_finger();
//     } else {
//       Serial.println("Invalid command.");
//     }

//     resetSensor();
//     isProcessing = false;
//   }
// }

// void sendFingerprintTemplate() {
//     int retryCount = 0; 
//     while (retryCount < 3) {
//         int result = finger.getImage();
//         if (result == FINGERPRINT_OK) {
//             Serial.println("Fingerprint found successfully!");
//             break;
//         } else {
//             Serial.println("No finger detected. Retrying...");
//             delay(1000);
//             retryCount++;
//         }
//     }

//     if (retryCount >= 3) {
//         Serial.println("Max retry limit reached. Exiting...");
//         return;
//     }

//     if (finger.image2Tz(1) != FINGERPRINT_OK) {
//         Serial.println("Failed to convert image to template.");
//         return;
//     }

//     delay(2000);  // Small delay for the second scan
//     Serial.println("Place finger on sensor again...");
//     while (finger.getImage() != FINGERPRINT_OK) {
//         delay(600);
//     }

//     if (finger.image2Tz(2) != FINGERPRINT_OK) {
//         Serial.println("Failed to convert second image.");
//         return;
//     }

//     if (finger.createModel() != FINGERPRINT_OK) {
//         Serial.println("Failed to create fingerprint model.");
//         delay(2000);
//         return;  // Exit after retry fails
//     }

//     if (finger.getModel() == FINGERPRINT_OK) {
//         Serial.println("BEGIN");
//         uint8_t model[534]; // Template size

//         delay(500);
//         for (int i = 0; i < 534; i++) {
//             while (!mySerial.available()) {
//                 delay(1);
//             }
//             model[i] = mySerial.read();
//         }
//         char encodedData[Base64.encodedLength(534)];
//         memset(encodedData, 0, sizeof(encodedData));
//         Base64.encode(encodedData, (char *)model, 534);
//         Serial.println("Encoded Data:");
//         Serial.write(encodedData);  // Send all encoded bytes
//         delay(1500);
//         Serial.println("FINISH");
//     } else {
//         Serial.println("Failed to retrieve fingerprint model data.");
//     }
// }



#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Base64.h>  

SoftwareSerial mySerial(2, 3);  // ports on the arduino for connection
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(9600);  // Communication with Python
  finger.begin(57600);  // Communication with fingerprint sensor

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found.");
  } else {
    Serial.println("Fingerprint sensor not found.");
    while (1);  // wait if sensor is not found
  }
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command) {
      if (command == "VERIFY_FINGER"){
      Serial.println("Starting verification...");
      verify_finger();
      
      Serial.println("we are here");
    } else if (command == "SCAN_FINGER") {
      Serial.println("scanning finger now...");
      sendFingerprintTemplate();
      resetSensor();
    } 
    else {
      Serial.println("Invalid command. Failed to go to a method.");
    }
    }
  }
  delay(700);  // Small delay
}



void clearSerialBuffer() {
  while (mySerial.available()) {
    mySerial.read();
  }
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

      delay(2000);  // small delay for the second scan

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
        uint8_t model[534]; // template size
        finger.getModel();  // Prepare data to be read
        for (int i = 0; i < 534; i++) {
          model[i] = mySerial.read();  // Collect the bytes
        }
        Serial.println("we are after the for loop");

        // Base64 encode full data
        for (int i = 0; i < 534; i += 128) {  // Process in chunks
        char chunkEncoded[Base64.encodedLength(128)];
        Base64.encode(chunkEncoded, (char *)model + i, min(128, 534 - i));
        Serial.write(chunkEncoded);
        delay(50);  // Allow some time for the host to process
        }


        // Serial.write(encodedData);  // Send all encoded bytes
        delay(1500);
        Serial.println("FINISH");
      } else {
        Serial.println("Failed to retrieve fingerprint model data.");
        sendFingerprintTemplate();  // Retry if fail
      }
      return;
    } else {
      Serial.println("No finger detected. Retrying...");
      delay(500);
    }
  }
}




void verify_finger() {
    clearSerialBuffer();
    Serial.println("Inside verify_finger() method");
    delay(500);

    // Capture finger image
    int result;
    while (true) {
        result = finger.getImage();
        if (result == FINGERPRINT_OK) break;
        delay(500);
    }

    if (finger.image2Tz() != FINGERPRINT_OK) {
        Serial.println("Couldn't convert the image...");
        return;
    }

    result = finger.fingerFastSearch();
    if (result == FINGERPRINT_OK) {
        Serial.println("Found a matching print... verified");
        Serial.println(finger.fingerID);
    } else {
        Serial.println("No finger could be recognized");
    }

    uint8_t model[534];
    if (finger.getModel() == FINGERPRINT_OK) {
        Serial.println("Sending template for verification...");
        
        for (int i = 0; i < 534; i++) {
            while (!mySerial.available()) delay(10);
            model[i] = mySerial.read();
        }

        for (int i = 0; i < 534; i += 128) {  // Process in chunks
        char chunkEncoded[Base64.encodedLength(128)];
        Base64.encode(chunkEncoded, (char *)model + i, min(128, 534 - i));
        Serial.println("BEGIN");
        Serial.println(chunkEncoded);
        delay(50);  // Allow some time for the host to process
        }

        
        // Serial.println(chunkEncoded);
        delay(50);
        
        Serial.println("FINISH");
        resetSensor();
      }else {
        Serial.println("Failed to retrieve fingerprint model.");
    }
}


void resetSensor() {
  mySerial.end();      // End communication
  delay(100);
  finger.begin(57600);  // Restart communication
  clearSerialBuffer();  // Ensure buffer is clean
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


//using the getmodel once code
// if (finger.getModel() == FINGERPRINT_OK) {
//     Serial.println("BEGIN");

//     // Prepare to read model data
//     uint8_t model[534];  // Template size
//     for (int i = 0; i < 534; i++) {
//         while (!mySerial.available()) {
//             // Wait for data to be available
//         }
//         model[i] = mySerial.read();  // Collect the bytes
//     }

//     // Base64 encode full data
//     char encodedData[Base64.encodedLength(534)];
//     Base64.encode(encodedData, (char *)model, 534);

//     // Send encoded data
//     Serial.write(encodedData);
//     delay(1500);  // Wait for transmission to complete
//     Serial.println("FINISH");
// } else {
//     Serial.println("Failed to retrieve fingerprint model.");
// }





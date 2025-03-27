
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
  // deleteit();
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command) {
      if (command == "VERIFY_FINGER"){
      Serial.println("Starting verification...");
      store_template();
      delay(1000);
      getFingerprintID();
      delay(1000);
      deleteit();
    } else if (command == "SCAN_FINGER") {
      Serial.println("scanning finger now...");
      sendFingerprintTemplate();
    } else if (command == "READ"){
      deleteit();
    } 
    else {
      Serial.println("Invalid command. Failed to go to a method.");
    }
    }
  }
  delay(700);  // Small delay
}



void clearSerialBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}


//original one
void sendFingerprintTemplate() {
  
  while (true) {
    // int result = finger.getImage();
    while (finger.getImage() != FINGERPRINT_OK) {
      Serial.println("fingerprint not found");
      delay(10);
    }

    if (finger.image2Tz(1) != FINGERPRINT_OK) {
      Serial.println("Failed to convert image to template.");
      return;
    }
    Serial.println("remove finger");
    delay(2000);  // small delay for the second scan

    
    while (finger.getImage() != FINGERPRINT_OK) {
      Serial.println("Place finger on sensor again...");
      // delay(10);
    }
    

    if (finger.image2Tz(2) != FINGERPRINT_OK) {
      Serial.println("Failed to convert second image.");
      return;
    }
    
    Serial.println("remove finger");
    delay(2000);
    while(finger.getImage() != FINGERPRINT_OK){
      Serial.println("place finger on sensor again please");
      // delay(10);
    }

    if (finger.image2Tz(3) != FINGERPRINT_OK){
      Serial.println("failed to convert the image");
      return;
    }

    Serial.println("remove finger");
    delay(2000);

    while(finger.getImage() != FINGERPRINT_OK){
      Serial.println("place finger on sensor again please");
      // delay(10);
    }
    
    if (finger.image2Tz(4) != FINGERPRINT_OK){
      Serial.println("fingerprint failed to convert");
      return;
    }

    int a = finger.createModel();
    if (a != FINGERPRINT_OK) {
      Serial.println("Failed to create fingerprint model.");
      Serial.println(a);
      delay(1000);
      continue;  // Retry
    }
    int id = 0;
    int next = id++;

    finger.storeModel(1);
    finger.getTemplateCount();
    Serial.println(finger.templateCount);

    a = finger.getModel();
    
    if (a == FINGERPRINT_OK) {
      Serial.println("BEGIN");
      
      // Sending model data in chunks
      uint8_t model[512]; // template size
      if (a == 0){  // Prepare data to be read
      for (int i = 0; i < 512; i++) {
        model[i] = mySerial.read();  // Collect the bytes
        
        }
      }

      for (int i = 0; i < 512; i++) {
        Serial.write(model[i]);  // Send raw bytes one by one
        delay(10);
      }

      delay(1600);
      Serial.println("FINISH");
    } else {
      Serial.println("Failed to retrieve fingerprint model data.");
      sendFingerprintTemplate();  // Retry if fail
    }
    return;
    
  
}
}



void store_template() {
      // Notify Python that Arduino is ready
      delay(10);
      Serial.println("READY");
      Serial.flush();
      uint8_t buffer[512];  // Buffer for fingerprint data
      int index = 0;
      bool receiving = true;  // Flag to indicate receiving data
      
      // Serial.println("Waiting for fingerprint data...");
      while (receiving) {  // Keep reading bytes until newline is detected
          if (Serial.available() > 0) {  
              uint8_t c = Serial.read();  // Read a byte

              // Stop receiving if newline character is detected
              if (c == '\n') {
                  
                  receiving = false;  // Stop reading data
                  Serial.println("");
                  Serial.println(index);
                  delay(2000);
                  
                  Serial.println(" ");
                  Serial.print("received lines: ");
                  for (int i = 0; i < index; i++){
                        Serial.write(buffer[i]);
                      }
                  

                  Serial.println("Fingerprint received!");
                  Serial.println(" ");
                  delay(100);

                  uint8_t result = finger.createModel();
                  if (result != FINGERPRINT_OK) {
                  Serial.print("Failed to create fingerprint model!");
                  return;  // Exit if model creation fails
                  }

                  result = finger.storeModel(buffer);
                  if (result == FINGERPRINT_OK) {
                      Serial.println("Fingerprint successfully uploaded to the scanner!");
                  } 
                  
                  int id = finger.getTemplateCount() + 1;
                  result = finger.storeModel(id);
                  if (result != FINGERPRINT_OK){
                    Serial.print("couldn't save print in the sensor");
                    return;
                  }
                  else{
                    Serial.println("saved the print");
                  }

                  delay(700);
                  finger.getTemplateCount();
                  Serial.println(finger.templateCount);


                  // Notify Python that fingerprint was received and stored
                  Serial.println("RECEIVED");
                  Serial.println("STORED");
                  Serial.println(finger.templateCount);
                  delay(1500);
                  memset(buffer, 0, sizeof(buffer));
                  index = 0;
                  
                }

              // Store only if there's space
              if (index <= sizeof(buffer)) {
                  buffer[index++] = c;
                  // Serial.print(index);
                  // delay(10);
              
              } else if(index > sizeof(buffer)){
                Serial.println("you fucked up index is too big");
                return;
              }
                else {
                  Serial.print(" ERROR: Buffer overflow!");
                  return;
              }
              
          }

          delay(10);  // Small delay to allow buffer processing
      
    }
}

uint8_t getFingerprintID() {
  clearSerialBuffer();
  delay(1000);
  Serial.println("now comparing the print");
  bool running = true;

  while (running){
    if (Serial.available() > 0){
      String lines = Serial.readStringUntil('\n');
      if (lines == "continue"){

        while (finger.getImage() != FINGERPRINT_OK){
          Serial.println("place finger on the sensor...");
          delay(50);
        }

        if (finger.image2Tz() == FINGERPRINT_OK){
          Serial.println("image made");
        }

        if (finger.fingerSearch() == FINGERPRINT_OK) {
          Serial.println("Found a print match!");
          Serial.print("Found ID #"); Serial.print(finger.fingerID);
          Serial.print(" with confidence of "); Serial.println(finger.confidence);
          Serial.println("END");
        }
        else {
          Serial.println("INTRUDER");
        }

        // found a match!
        
        running = false;

        return finger.fingerID;
      }
    }
  }
}


// uint8_t getFingerprintID() {
//     delay(1000);
//     Serial.println("Now comparing the print");

//     // Wait for input
//     if (Serial.available() > 0) {
//         String input = Serial.readStringUntil('\n'); // Read the full input line
        
//         if (input == "c") { // Check if input is 'c'
//             bool answer = true;
            
//             while (answer) {
//                 uint8_t p = finger.getImage();
//                 if (p != FINGERPRINT_OK) {
//                     Serial.println("Place finger on the sensor...");
//                     delay(50);
//                     continue;
//                 }

//                 p = finger.image2Tz();
//                 if (p != FINGERPRINT_OK) {
//                     Serial.println("Error converting image!");
//                     continue;
//                 }

//                 Serial.println("Image made!");

//                 // Search for a match
//                 p = finger.fingerSearch();
//                 if (p == FINGERPRINT_OK) {
//                     Serial.print("Found ID #"); 
//                     Serial.print(finger.fingerID);
//                     Serial.print(" with confidence of "); 
//                     Serial.println(finger.confidence);
//                     Serial.println("END");
//                     return finger.fingerID;  // Return valid ID
//                 } else {
//                     Serial.println("INTRUDER DETECTED!");
//                     return 0xFF;  // Return invalid ID if no match
//                 }
//             }
//         }
//     }

//     return 0xFF;  // Return invalid ID if no input received
// }


// returns -1 if failed, otherwise returns ID #
// int getFingerprintIDez() {
//   uint8_t p = finger.getImage();
//   if (p != FINGERPRINT_OK)  return -1;

//   p = finger.image2Tz();
//   if (p != FINGERPRINT_OK)  return -1;

//   p = finger.fingerFastSearch();
//   if (p != FINGERPRINT_OK)  return -1;

//   // found a match!
//   Serial.print("Found ID #"); Serial.print(finger.fingerID);
//   Serial.print(" with confidence of "); Serial.println(finger.confidence);
//   return finger.fingerID;
// }  



uint8_t deleteit(){
  uint8_t result = finger.emptyDatabase();

// if (result == FINGERPRINT_OK) {
//     Serial.println("All fingerprints deleted.");
// } else {
//     Serial.println("Failed to clear fingerprint database.");
// }
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


// using the getmodel once code
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





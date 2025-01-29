
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
      // verify_finger();
      // compareTemplate();
      store_template();
      
      // Serial.println("we are here");
    } else if (command == "SCAN_FINGER") {
      Serial.println("scanning finger now...");
      sendFingerprintTemplate();
    } else if (command == "READ"){
      Serial.println("going to the read function");
      // read();
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
    int result = finger.getImage();
    if (result == FINGERPRINT_OK) {
      Serial.println("fingerprint found successfully");
      

    if (finger.image2Tz(1) != FINGERPRINT_OK) {
      Serial.println("Failed to convert image to template.");
      return;
    }
    Serial.println("remove finger");
    delay(2000);  // small delay for the second scan

    
    while (finger.getImage() != FINGERPRINT_OK) {
      Serial.println("Place finger on sensor again...");
      // delay(400);
    }
    

    if (finger.image2Tz(2) != FINGERPRINT_OK) {
      Serial.println("Failed to convert second image.");
      return;
    }
    
    Serial.println("remove finger");
    delay(2000);
    while(finger.getImage() != FINGERPRINT_OK){
      Serial.println("place finger on sensor again please");
      // delay(400);
    }

    if (finger.image2Tz(3) != FINGERPRINT_OK){
      Serial.println("failed to convert the image");
      return;
    }

    Serial.println("remove finger");
    delay(2000);

    while(finger.getImage() != FINGERPRINT_OK){
      Serial.println("place finger on sensor again please");
      // delay(400);
    }

    if (finger.image2Tz(4) != FINGERPRINT_OK){
      Serial.println("fingerprint failed to convert");
      return;
    }

    int a = finger.createModel();
    if (a != FINGERPRINT_OK) {
      Serial.println("Failed to create fingerprint model.");
      delay(1000);
      continue;  // Retry
    }
    finger.storeModel(a);
    Serial.println("stored the print");

    int w = finger.getModel();
    // Serial.println(w);
    if (w == FINGERPRINT_OK) {
      Serial.println("BEGIN");

      // Sending model data in chunks
      uint8_t model[512]; // template size
      if (w == 0);{  // Prepare data to be read
      for (int i = 0; i < 512; i++) {
        model[i] = mySerial.read();  // Collect the bytes
        // Serial.print(w);
        }
      }

      for (int i = 0; i < 512; i++) {
        Serial.write(model[i]);  // Send raw bytes one by one
        // Serial.println(model[i], HEX);
      }

      delay(1600);
      Serial.println("FINISH");
    } else {
      Serial.println("Failed to retrieve fingerprint model data.");
      sendFingerprintTemplate();  // Retry if fail
    }
    return;
    }
      else {
      Serial.println("No finger detected. Retrying...");
      // delay(50);
    }
  
}
}

void store_template() {
    Serial.println("READY");  // Notify Python that Arduino is ready
    
    uint8_t buffer[512];  // Buffer for fingerprint data
    int index = 0;
    String receivedData = "";  // Store incoming data to check for "END"

    while (true) {  
        if (Serial.available() > 0) {  
            uint8_t c = Serial.read();  
            // if(Serial.read() == "ready"){
            //   Serial.println("READY");
            //   Serial.flush();
            // } 
            buffer[index++] = c;         
            receivedData += (char)c;     

            // Stop receiving if we detect "END" at the end
            if (receivedData.endsWith("END")) {
                Serial.println("RECEIVED");  // Notify Python that data is received
                Serial.print("Buffer content: ");

                // Print fingerprint data in hex format
                for (int i = 0; i < index - 3; i++) {  // Ignore "END"
                    Serial.print(buffer[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
                Serial.println("hello");
                if (Serial.read() == "stop"){
                  break; } // Stop receiving data
            }

            // Prevent overflow
            if (index >= sizeof(buffer)) {
                Serial.println("ERROR: Buffer overflow!");
                break;
            }
        }
        // Serial.print(sizeof(buffer));
        delay(10);
    }
}


// void store_template() {
//     Serial.println("We are in the store method");
    
//     uint8_t buffer[515];  // Array to store fingerprint data (assuming 512 bytes)
//     int index = 0;        // To keep track of where you are in the array
//     String receivedData = ""; // Store incoming data as a string to check for "END"

//     while (true) {  // Infinite loop until "END" is found
//         if (Serial.available() > 0) {  // Check if data is available
//             uint8_t c = Serial.read();   // Read one byte from Serial
//             buffer[index++] = c;         // Append it to the buffer
//             receivedData += (char)c; 
//             Serial.println(receivedData);    // Append to the string

//             // Stop receiving if we detect "END" at the end
//             if (receivedData.endsWith("END")) {
//                 Serial.println("End of data detected!");
//                 Serial.print("Buffer content: ");

//                 // Print the buffer content in hexadecimal format
//                 for (int i = 0; i < index - 3; i++) {  // Ignore "END" bytes
//                     Serial.print(buffer[i], HEX);
//                     Serial.print(" ");
//                 }
            
//                 Serial.println();
//                 break;  // Exit the loop after receiving the full data
//             }

//             // Prevent overflow
//             if (index > sizeof(buffer)) {
//                 Serial.println("Buffer overflow! Too much data.");
//                 break;
//             }
//         }
//         delay(10);  // Optional delay to prevent excessive CPU usage
//     }
// }







// void compareTemplate(){
//   if (!templateUploaded){
//     sentTemplates();
//   }

//   if (templateUploaded){
//     Serial.println("place finger on scanner ");

//     if (finger.getImage() == FINGERPRINT_OK){
//       Serial.println("image taken");
//     }
//     while (finger.image2Tz(1) != FINGERPRINT_OK){
//       Serial.println("couldnt take image");
//       delay(200);
//     }
    
//     if (finger.fingerFastSearch() == FINGERPRINT_OK){
//       Serial.println("match found");
//     }
//     if (finger.loadModel(2) == FINGERPRINT_OK){
//       Serial.println("template loaded into buffer");

//       int match = finger.fingerFastSearch();

//       if (match == FINGERPRINT_OK){
//         Serial.println("match found");
//       }
//       else{
//         Serial.println("nothing found to match ");
//       }
//     } else{
//       Serial.println("failed to load template");
//     }

//   }
// }

// void sentTemplates() {
//   Serial.println("Waiting to receive template from Python...");
//   receivedBytes = 0;

//   while (receivedBytes < templateSize) {
//     if (Serial.available()) {
//       templateBuffer[receivedBytes++] = Serial.read();
//     }
//   }

//   if (receivedBytes == templateSize) {
//     Serial.println("Template received. Uploading to sensor...");
//     if (finger.loadModel(templateBuffer) == FINGERPRINT_OK) {
//       Serial.println("Template uploaded successfully.");
//       templateUploaded = true;
//     } else {
//       Serial.println("Failed to upload template.");
//       templateUploaded = false;
//     }
//   }
// }




void verify_finger() {
    clearSerialBuffer();
    Serial.println("Inside verify_finger() method");
    delay(500);
    String recieved_print = "";
    // stored_print = "";
    Serial.println("we are just before the while loop");
    while (true){
      if (Serial.available() > 0){
        String c = Serial.readStringUntil("\n");
        delay(700);
        Serial.println("we are after the serial read");
        if (c == '\n'){
          Serial.print("recieved so far");
          Serial.println(recieved_print +'\n');
          Serial.println("print recieved");
          delay(100);

          // if (store_template(recieved_print,2)){
          //   Serial.println("template loaded into memory");

            while (true){
              Serial.println("place finger on the sensor");
              int result = finger.getImage();
              if (result == "FINGERPRINT_OK"){
                Serial.println("scan was a success");

                if (finger.image2Tz(1) == FINGERPRINT_OK){
                  int match = finger.fingerFastSearch();
                  if (match == "FINGERPRINT_OK"){
                    Serial.println("MATCH");
                  }
                  else{
                    Serial.println("NO PRINT FOUND");
                  }
                  break;
                }else{
                  Serial.println("couldnt convert template");
                }
              }
              else{
                Serial.println("NO PRINT FOUND");
              } delay(500);
            }
          }
          else{
            Serial.println("no template loaded");
          } recieved_print = "";
            break;
        }
        // else{
        //   recieved_print += c;
        // }
      }
    }
    
// }



// uint8_t deleteit(){
//   uint8_t result = finger.emptyDatabase();

// if (result == FINGERPRINT_OK) {
//     Serial.println("All fingerprints deleted.");
// } else {
//     Serial.println("Failed to clear fingerprint database.");
// }
// }









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





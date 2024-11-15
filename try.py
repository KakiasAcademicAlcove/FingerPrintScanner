import base64
import serial
import time
from cryptography.fernet import Fernet
import yaml
import uuid
import mysql.connector
from datetime import datetime, timezone

class scanner:

    def __init__(self):
        self.key = Fernet.generate_key()
        self.cipher_suite = Fernet(self.key)
        self.db = mysql.connector.connect(
            username = "newuser",
            password = "Password",
            host = "localhost",
            database = "fingerprint"
        )
        self.cursor = self.db.cursor()
        try:
            self.arduino = serial.Serial('/dev/cu.usbmodem101', 9600, timeout=1)  # Add timeout for better handling
            time.sleep(2)  # Allow time for the connection to establish
        except serial.SerialException as e:
            print(f"Failed to connect to the serial port: {e}")
            self.arduino = None
            
    def start(self):
        starting = input("Do you want to enroll a finger? y/yes ").lower()
        if starting == "yes" or starting == "y":
            self.sendCommand()
        else:
            print("Exiting...")
            return

    def sendCommand(self):
        # Send command to initiate fingerprint scan and data transmission
        command = "SCAN_FINGER\n"
        print(f"Sending command: {command.strip()}")
        self.arduino.write(command.encode())  # Send the command as bytes
        time.sleep(0.5)
        random = uuid.uuid4()
        string_id = random.hex

        while True:
            outcome = self.arduino.readline().decode("utf-8").strip()  # Decode the incoming line
            if outcome == "BEGIN":
                time.sleep(0.5)
                print("now going to the next function ======")
                fingerData = self.readFinger()
                if fingerData != "Failed to create fingerprint model.":
                    print("finger data has been taken ==========")
                    encryptedData = self.cipher_suite.encrypt(fingerData)
                    print(f"The encrypted data is: {encryptedData}")
                    name = input("what is the name of this account ".upper())
                    self.save_to_database(encryptedData, name)
                    # with open("finger_data.yaml", "a") as file:
                    #         yaml.dump({name + string_id: encryptedData}, file, default_flow_style=False)
                    #         yaml.dump("\n")
                    print(f"saved fingerprint for {name} to database ".upper())
                elif fingerData == "Failed to create fingerprint model.":
                    self.sendCommand()
                else:
                    print("No data received.")
                break
            
            elif outcome:
                print(f"Arduino output: {outcome}")
                    
            else:
                # print("No outcome from the Arduino.")
                time.sleep(0.1)


    def readFinger(self, timeout=10):
        start_time = time.time()
        data = ""
        capturing = True
        print("read finger method has started")

        if not self.arduino.is_open:
            print("Serial port is not open. Reopening...")
            self.arduino.open()  # Open the serial port again if it's closed

        print("now starting the reading ====")
        while capturing is True:
            line = self.arduino.readline().decode("utf-8").strip()

            # Print out each line received for debugging
            print(f"Received line: {line}")
            # decoded = base64.b64decode(line).strip()
            # print(decoded)
            
            if line == "FINISH":
                print("Finished receiving fingerprint data.")
                capturing = False
                decoded_data = base64.b64decode(data)
                self.arduino.close()
                return decoded_data  # Binary data

            elif capturing and line:
                data += line  # Collect Base64 string
            
            
            if time.time() - start_time > timeout:
                print("Timeout while waiting for fingerprint data.")
                break

        return None  # In case of timeout


    def closeSerial(self):
        if self.arduino.is_open:
            self.arduino.close()

    def save_to_database(self,encrypted_data, name):
        if self.db.is_connected():
            print("SYSTEM IS CONNECTED TO THE DATABASE TO SAVE ")
        else:
            print("NO CONNECTION MADE ")
        self.cursor.execute("""
        CREATE TABLE IF NOT EXISTS users (
            user_id INT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(255) NOT NULL UNIQUE,
            encrypted_fingerprint LONGBLOB NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        time = datetime.now(timezone.utc)  
        command = "INSERT INTO users (username, encrypted_fingerprint, created_at) VALUES (%s, %s, %s)"
        values = (name, encrypted_data, time)
        self.cursor.execute(command , values)
        self.db.commit()
        


    def verify_finger(self, fingerprint):
        pass

# Instantiate and run
run = scanner()
run.start()
run.closeSerial()





# To decrypt later:
# decrypted_data = cipher_suite.decrypt(encrypted_data)



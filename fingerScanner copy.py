import serial
import time
from cryptography.fernet import Fernet
import mysql.connector
from datetime import datetime, timezone
import math



class Scanner:
    def __init__(self):
        self.db = mysql.connector.connect(
            username="newuser",
            password="Password",
            host="localhost",
            database="fingerprint",
            charset='latin1'
        )
        
        self.cursor = self.db.cursor()

        # Generate a Fernet encryption key (you should save this key securely)
        self.encryption_key = Fernet.generate_key()
        self.cipher = Fernet(self.encryption_key)

        # print(self.cipher)
        # print(self.cipher)
        # print(self.cipher)
        # print(self.encryption_key)

        try:
            self.arduino = serial.Serial('/dev/cu.usbmodem101', 9600, timeout=1)  # Add timeout for better handling
            time.sleep(2)  # Allow time for the connection to establish
        except serial.SerialException as e:
            print(f"Failed to connect to the serial port: {e}")
            self.arduino = None

        self.cursor.execute("""
        CREATE TABLE IF NOT EXISTS users (
            user_id INT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(255) NOT NULL UNIQUE,
            encrypted_fingerprint LONGBLOB NOT NULL,
            encryption_key TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)



    def start(self):
        while True:
            action = input("Do you want to enroll a finger? 'enroll/e' or verify 'verify/v' ").lower()
            if action in ["enroll", "e"]:
                time.sleep(0.5)
                self.enroll_finger()
            elif action in ["verify", "v"]:
                time.sleep(0.5)
                self.verify_finger()
            elif action in ["q"]:
                print("Deleted the fingerprints")
                self.arduino.write(b"READ")
                # self.read()
            else:
                print("Invalid command, try again")
                time.sleep(1)
                False

    def enroll_finger(self):
        if not self.arduino.is_open:
            self.arduino.open()

        command = "SCAN_FINGER\n"
        print(f"Sending command: {command.strip()}")
        self.arduino.write(command.encode())  # Send the command as bytes
        time.sleep(0.5)

        while True:
            outcome = self.arduino.readline().strip()
            print(outcome)
            if outcome == b"BEGIN":
                finger_data = self.read_finger()
                if finger_data:
                    print(finger_data)
                    print("Fingerprint data captured.")
                    encrypted_data = self.encrypt_data(finger_data)
                    print(encrypted_data)
                    username = input("Enter a username for this fingerprint: ").strip()
                    self.save_to_database(encrypted_data, username)
                    break
                else:
                    print("Failed to capture fingerprint data.")
                    break
#original one
    def read_finger(self, timeout=100):
        start_time = time.time()
        data = b""  # Collect data as raw bytes
        while True:
            line = self.arduino.readline().strip()
            print(line)
            if line:
                    decoded_line = line
                    if decoded_line == b"FINISH":
                        print("Finished receiving fingerprint data.")
                        print(len(data))
                        return data
            data += line  # Append raw binary data
            print(type(line))
            if time.time() - start_time > timeout:
                print("Timeout while waiting for fingerprint data.")
                break
        return None  # Return None in case of timeout

    def encrypt_data(self, data):
        """Encrypt binary fingerprint data using Fernet."""
        return self.cipher.encrypt(data)

    def decrypt_data(self, encrypted_data):
        """Decrypt binary fingerprint data using Fernet."""
        return self.cipher.decrypt(encrypted_data)

    def save_to_database(self, encrypted_data, username):
        if self.db.is_connected():
            print("Connected to the database.")
        else:
            print("Database connection failed.")
            return  # Exit if the database connection fails

        # Ensure unique username
        while True:  # Loop until a unique username is entered
            query = "SELECT COUNT(*) FROM users WHERE username = %s"
            self.cursor.execute(query, (username,))
            row = self.cursor.fetchone()

            if row[0] > 0:
                print("Username already in use, choose another.")
                username = input("Enter a new username: ").strip()  # Ask for new username
            else:
                # Insert fingerprint data into the database
                created_at = datetime.now(timezone.utc)
                command = "INSERT INTO users (username, encrypted_fingerprint, encryption_key, created_at) VALUES (%s, %s, %s, %s)"
                self.cursor.execute(command, (username, encrypted_data, self.encryption_key, created_at))
                self.db.commit()
                print(f"Fingerprint for {username} saved successfully.")
                break  # Exit loop once the username is successfully saved


    
    def verify_finger(self):
        # open serial if its not
        if not self.arduino.is_open:
            self.arduino.open()
        
        # print(users) 

        #make the command to activate ardunio
        command = "VERIFY_FINGER\n"
        print(f"Sending command: {command.strip()}")
        self.arduino.write(command.encode())  # Send the command as bytes
        time.sleep(1)
        print("Waiting for fingerprint scan...")
        
        #read what ardunio says and print it out
        outcome = self.arduino.readline()
        print(outcome)


        uname = input("enter your username\n")
        self.cursor.execute("SELECT username, encrypted_fingerprint, encryption_key FROM users WHERE username = %s", (uname,))
        users = self.cursor.fetchall()
        
        for username, encrypted_data, encryption_key in users:
            # Decrypt fingerprint data
            cipher = Fernet(encryption_key)
            stored_fingerprint = cipher.decrypt(encrypted_data)
            print(len(stored_fingerprint))
            # stored_fingerprint = bytearray(stored_fingerprint)

            store = self.sendToArduino(stored_fingerprint, username)
            # print(store)
            # if store:
            #     print(store)
                
                    
                
    def compare(self, username):
        print("in the compare function")  
        print(username)
        time.sleep(5)

    # def sendToArdunio(self, stored_print, username):
    #     if not self.arduino.is_open:
    #         self.arduino.open()
    #     chunk = 64
    #     numOfChunks = len(stored_print) // chunk +1
    #     print(f"Sending fingerprint data for {username} in {numOfChunks} chunks.")
    #     # print(len(str(stored_print)))
    #     reply = self.arduino.readline().strip().decode()
    #     print(reply)
        
    #     # self.arduino.write("BEGIN\n".encode())
    #     array = bytearray()
    #     for i in range(numOfChunks):
            
    #         start = i * chunk
    #         end = (i+1) * chunk
    #         full = stored_print[start:end]
    #         sending = self.arduino.write(full)
    #         if sending:
    #             replies = self.arduino.readline().strip().decode()
    #             print(replies)
            
    #         array.extend(bytes(full))
    #         # print(full)
    #         print(f"sent {i +1} / {numOfChunks}")
    #         time.sleep(0.5)
    #         # print(full)
    #         if b"END" in full:
    #             print(replies)
    #             # return array
    #         else:
    #             print("written to the arduino")
    #             # self.arduino.write("written to the arduino".encode())
    #             time.sleep(1)


    def sendToArduino(self, stored_print, username):
        if not self.arduino.is_open:
            self.arduino.open()
        while True:
            reply = self.arduino.readline().strip().decode()
            print(reply)
            time.sleep(1)
            # self.arduino.write(b"ready")

            stored_print = bytearray(stored_print)
            chunk_size = 64
            num_of_chunks = math.ceil(len(stored_print) / chunk_size)
            print(f"Waiting for Arduino to be ready...")

            # Wait for Arduino to say "READY"
            
            if reply == "READY":
                time.sleep(0.5)
                print("Arduino is ready to receive fingerprint data.")
                break
            
            time.sleep(1.5)

        print(f"Sending fingerprint data for {username} in {num_of_chunks} chunks.")
        
        for i in range(num_of_chunks):
            start = i * chunk_size
            end = (i + 1) * chunk_size
            chunk_data = stored_print[start:end]

            self.arduino.write(chunk_data)
            # print(f"Chunk {i+1}/{num_of_chunks}: {len(chunk_data)} bytes")
            print(f"Chunk {i+1}/{num_of_chunks}: {len(chunk_data)} bytes - {chunk_data.hex()}")
            time.sleep(1)
        self.arduino.write(b"\n")

        self.matching(username)

    def matching(self, username):
        if not self.arduino.is_open:
            self.arduino.open()
        # Wait for Arduino to confirm receipt
        while True:
            response = self.arduino.readline().strip()
            print(response)
            if b"now comparing the print" in response:
                continues = input("continue when ready | type 'c'".lower())
                if continues == "c":
                    self.arduino.write(b"continue\n")
                    while True:
                        responses = self.arduino.readline().strip()
                        print(responses)
                        if b"END" in responses:
                            print(f"the fingerprint is a match for {username} !!!!")
                            return
                        elif b"INTRUDER" in responses:
                            print("no matching fingerprint, youre not who you say youre you snake !!!")
                            return
                        elif b"All fingerprints delete" in responses:
                            self.start()
                else:
                    print("invalid command")
            

        

    def close(self):
        """Close the database and serial connections."""
        if self.db.is_connected():
            self.cursor.close()
            self.db.close()
        if self.arduino and self.arduino.is_open:
            self.arduino.close()
            

    def read(self):
        if not self.arduino.is_open:
            self.arduino.open()
        command = "VERIFY_FINGER\n"
        self.arduino.write(command.encode())
        print(f"sending command: {command}")
        
        while True:
            line = self.arduino.readline().decode().strip()
            if line:
                print(line)
            respond = self.arduino.readline().decode().strip()
            if respond:
                print(respond)
                if respond == "END":
                    break
            self.arduino.write("END\n".encode())
            
             

# Instantiate and run
scanner = Scanner()
scanner.start()

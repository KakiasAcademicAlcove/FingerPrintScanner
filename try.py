import binascii
import numpy as np
import serial
import time
from cryptography.fernet import Fernet
import mysql.connector
from datetime import datetime, timezone



#this needs to have functions that can retrive each print and save them in a variable then also a function to send them to the arduino/n
# maybe have some if statement that makes it wait for the ardunio to say no match before sending the next one ? that way it can dynamically load and compare  


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
                self.enroll_finger()
            elif action in ["verify", "v"]:
                self.verify_finger()
            # elif action in ["q"]:
            #     print("going to read now")
            #     self.read()
            else:
                print("Exiting...")
                break

    def enroll_finger(self):
        if not self.arduino.is_open:
            self.arduino.open()

        command = "SCAN_FINGER\n"
        print(f"Sending command: {command.strip()}")
        self.arduino.write(command.encode())  # Send the command as bytes
        time.sleep(0.5)

        while True:
            outcome = self.arduino.readline().decode("utf-8").strip()
            print(outcome)
            if outcome == "BEGIN":
                finger_data = self.read_finger().strip()
                if finger_data:
                    print("Fingerprint data (hex):")
                    print(' '.join(f'{byte:02x}' for byte in finger_data))
                    print("Fingerprint data captured.")
                    encrypted_data = self.encrypt_data(finger_data + b"END\n")
                    print(encrypted_data)
                    username = input("Enter a username for this fingerprint: ").strip()
                    self.save_to_database(encrypted_data, username)
                    break
                else:
                    print("Failed to capture fingerprint data.")
                    break
#original one
    def read_finger(self, timeout=10):
        start_time = time.time()
        data = b""  # Collect data as raw bytes
        while True:
            line = self.arduino.readline().strip()
            print(line)
            if line:
                try:
                    decoded_line = line.decode("utf-8")
                    if decoded_line == "FINISH":
                        print("Finished receiving fingerprint data.")
                        return data
                except UnicodeDecodeError:
                    pass
                data += line  # Append raw binary data
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

        if not self.arduino.is_open:
            self.arduino.open()
        command = "VERIFY_FINGER\n"
        print(f"Sending command: {command.strip()}")
        self.arduino.write(command.encode())  # Send the command as bytes
        time.sleep(2)
        print("Waiting for fingerprint scan...")
        # Wait for Arduino's response

        while True:
            outcome = self.arduino.readline().decode("utf-8").strip()
            print(outcome)

            # Read fingerprint data from database
            self.cursor.execute("SELECT username, encrypted_fingerprint, encryption_key FROM users")
            users = self.cursor.fetchall()
            
            for username, encrypted_data, encryption_key in users:
                # Decrypt fingerprint data
                cipher = Fernet(encryption_key.encode("utf-8"))
                stored_fingerprint = cipher.decrypt(encrypted_data)
                # print(stored_fingerprint)
                store = self.sendToArduino(stored_fingerprint, username)
                # print(store)
                if store:
                    self.compare(store)
                    
                # # Chunk the data before sending
                # print(f"Sending fingerprint data for {username} in {num_chunks} chunks.")

                    # ignore this
                    # for i in range(0, len(stored_fingerprint), chunk_size):
                    #     chunk = stored_fingerprint[i:i+chunk_size]
                    #     self.arduino.write(chunk)
                    #     time.sleep(0.1)  # Give Arduino time to process

                    # Send the data in chunks
                # chunk_size = 64
                # num_chunks = len(stored_fingerprint) // chunk_size + 1
                # template = bytearray()
                
                # for i in range(num_chunks):
                #     start_index = i * chunk_size
                #     end_index = (i + 1) * chunk_size
                #     chunk = stored_fingerprint[start_index:end_index]

                #     # Send the chunk
                #     time.sleep(1)
                #     # print("\n".encode())
                    
                #     template.extend(chunk)
                #     # print(chunk)
                #     print(f"Sent chunk {i + 1}/{num_chunks}")


                # if "\n".encode() in chunk:
                #     send = self.sendToArdunio(template)
                #     # print(template[::])
                #     # print(response)
                #     if send:
                #         response = self.arduino.readline()
                #         if response == "MATCH":
                #             print("found a match")
                #         elif response == "NO MATCH":
                #             print("no match found")
                #             return
                
                print("Fingerprint data sent to Arduino.")
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
    #         if self.arduino.write(full):
    #             replies = self.arduino.readline().strip().decode()
    #             print(replies)
            
    #         array.extend(bytes(full))
    #         # print(full)
    #         print(f"sent {i +1} / {numOfChunks}")
    #         time.sleep(1)
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
        self.arduino.write(b"ready")

        chunk_size = 64
        num_of_chunks = (len(stored_print) // chunk_size) + 1
        print(f"Waiting for Arduino to be ready...")

        # Wait for Arduino to say ready
        while True:
            reply = self.arduino.readline().strip().decode()
            if reply == "READY":
                print("Arduino is ready to receive fingerprint data.")
                break
        
        print(f"Sending fingerprint data for {username} in {num_of_chunks} chunks.")

        for i in range(num_of_chunks):
            start = i * chunk_size
            end = (i + 1) * chunk_size
            chunk_data = stored_print[start:end]

            if self.arduino.write(chunk_data): 
                print(f"Sent chunk {i + 1}/{num_of_chunks}")
                

            time.sleep(0.1)  

        # Send end to tell us full fingerprint is sent
        self.arduino.write(b"END")
        print("Sent END signal to Arduino")

        # Wait for Arduino to confirm it has it
        while True:
            response = self.arduino.readline().strip().decode()
            if response == "RECEIVED":
                print("Arduino confirmed fingerprint received.")
                response2 = self.arduino.readline().strip().decode()
                if response2:
                    print(response2)
                    
                    if response == "hello":
                        self.arduino.write("stop".encode())
                        print("breaking from loop")
                        break   


        
            


    def close(self):
        """Close the database and serial connections."""
        if self.db.is_connected():
            self.cursor.close()
            self.db.close()
        if self.arduino and self.arduino.is_open:
            self.arduino.close()
            

    def read(self):
        command = "READ\n"
        self.arduino.write(command.encode())
        print(f"sending command: {command}")
        
        while True:
            line = self.arduino.readline().decode().strip()
            if line:
                print(line)
            new_command = "@>v\xa2\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\n"
            self.arduino.write(new_command.encode())
            # print(f"sent the new command ========= : {new_command}")
            respond = self.arduino.readline().decode().strip()
            if respond:
                print(respond)
                if respond == "END":
                    break
            self.arduino.write("END\n".encode())
            
             

# Instantiate and run
scanner = Scanner()
scanner.start()

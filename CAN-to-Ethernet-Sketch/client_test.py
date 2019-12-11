import socket
import sys
import time
import traceback
import struct

CAN_MESSAGE_LENGTH = 25
MAX_UDP_SIZE = 1400 # This should match the programming on the node

PORT = 59581
node_adresses = {11:"192.168.1.2",
                  0:"192.168.1.5"}

# SOCK_DGRAM is the socket type to use for UDP sockets
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('',PORT))
server_socket.settimeout(0) #make it non-blocking

with open("drive_cycle.csv",'r') as drive_file:
    drive_lines = drive_file.readlines()


def process_message(message,address):
    print("Received Message of {} bytes".format(len(message)))
    if message[:5] == b'CAN2\xff':
        print(message)
        print("WTF?")
        sys.exit()
    if not message[:5] == b'TIME\xff':
        print(message)
        #for i in range(0,num_can_messages):
        #    CANID = struct.unpack(">L",message[1+i*CAN_MESSAGE_LENGTH:5+i*CAN_MESSAGE_LENGTH])[0]
        #    print("Received: {:08X}".format(CANID))

def send_drive_profile_change(drive_data):
    sequence = drive_data[0]
    rel_time = drive_data[1]
    commands = {}
    for i in range(len(drive_data[2:])//3): #use integer division
        try:
            source = int(drive_data[2+3*i])
            if source not in commands:
                commands[source]=""
            SPN = int(drive_data[3+3*i])
            value = float(drive_data[4+3*i])
            commands[source]+="{:d},{:0.3f},".format(SPN,value)
        except ValueError:
            pass
    for SA,command_string in commands.items():
        data_to_send = bytes(command_string[:-1],'ascii')
        if len(data_to_send) > 0 and len(data_to_send) < MAX_UDP_SIZE:
            bytes_sent = server_socket.sendto(data_to_send,(node_adresses[SA],PORT))
            print("Sent {} bytes: {}".format(bytes_sent, data_to_send))
                
while True:    
    previous_time = time.time()
    print("Drive Cycle Start Time: {:12.6f}".format(previous_time))
    for drive_line in drive_lines[2:]:
        drive_data = drive_line.strip().strip(',').split(",")
        sequence_number = int(drive_data[0])
        time_lapsed = float(drive_data[1])
        while True:
            current_time = time.time()
            time_delta = current_time-previous_time 
            if time_delta >= time_lapsed:
                #previous_time = current_time
                print("{:12.6f} ({:0.6f}) {}".format(current_time,time_delta,drive_data))
                send_drive_profile_change(drive_data)
                # Be sure to break from the eternal loop and get the next timestep
                break
            else:
                pass

            # Read the socket and process the message data
            # With the timeout set to zero, this is a non-blocking call. 
            # If no message is in the socket buffer, then a BlockingIOError is thrown.
            # This method helps process all the data in the socket. It is important not
            # to overload the network.
            try:
                message, address = server_socket.recvfrom(1500)
                process_message(message, address)   
            except BlockingIOError:
                pass
            

    

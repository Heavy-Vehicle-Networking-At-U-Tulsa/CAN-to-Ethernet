import socket
import sys
import time
import traceback

HOST, PORT = "192.168.1.2", 59581
data = b'Hello World'
sample_period = .1
# SOCK_DGRAM is the socket type to use for UDP sockets
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('',PORT))
server_socket.settimeout(0)

with open("drive_cycle.csv",'r') as drive_file:
	drive_lines = drive_file.readlines()


def process_message(message,address):
	if HOST in address:		
		#pass

		print("Received: ({}) {}".format(address,message))

def send_drive_profile_change(drive_data):
	data = bytes(repr(drive_data),'ascii')
	bytes_sent = server_socket.sendto(data,(HOST,PORT))
	print("Sent {} bytes: {}".format(bytes_sent, drive_data))
				
# As you can see, there is no connect() call; UDP has no connections.
# Instead, data is directly sent to the recipient via sendto().
while True:
	
	previous_time = time.time()
	for drive_line in drive_lines[1:]:
		drive_data = drive_line.strip().split(",")
		sequence_number = int(drive_data[0])
		time_lapsed = float(drive_data[1])
		while True:
			current_time = time.time()
			time_delta = current_time-previous_time 
			if time_delta >= time_lapsed:
				#previous_time = current_time
				print("{:12.6f} ({:0.6f}) {}".format(current_time,time_delta,drive_data))
				send_drive_profile_change(drive_data)
				# Be sure to break from the 
				break
			else:
				pass
			#time.sleep(.001)
			# Read the socket and process the message data
			# With the timeout set to zero, this is a non-blocking call. 
			# If no message is in the socket buffer, then a BlockingIOError is thrown.
			# This method helps process all the data in the socket. It is important not
			# to overload the network.
			try:
				while True:
					message, address = server_socket.recvfrom(1024)
					process_message(message, address)	
			except BlockingIOError:
				pass #print(traceback.format_exc())
			

	
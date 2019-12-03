import socketserver
import base64

def printFrame(canFrame):
    # Can Channel
    if canFrame[0] == '0x0':
        channel = "Can0"
    elif canFrame[0] == '0x01':
        channel = "Can1"
    else:
        channel = "Can?"
    # Timestamp
    timestamp = str(int(canFrame[1] + canFrame[2][2:], 16))
    if len(timestamp) == 4:
        timestamp = '0'+timestamp
    elif len(timestamp) ==3:
        timestamp = '00' + timestamp
    elif len(timestamp) == 2:
        timestamp = '000' + timestamp
    elif len(timestamp) == 1:
        timestamp + '000' + timestamp
    elif len(timestamp) == 0:
        timestamp = '00000'
    # Arbitration ID
    combID = int(canFrame[3] + canFrame[4][2:] + canFrame[5][2:] + canFrame[6][2:], 16)
    arbID = str(hex(combID >> 3))[2:].upper()
    # Extension Flags
    mask = 111
    ext = combID & mask
    extFlag = str(bin(ext))[2:]
    dlc = str(hex(int(canFrame[7], 16)))[2:]
    counter = 8
    data = ''
    while counter < 16:
        bite = str(hex(int(canFrame[counter], 16)))[2:]
        if len(bite) == 1:
            bite = '0' + bite
        counter += 1
        data = data + bite + ' '
    print(channel + '    ' + timestamp + '    ' + arbID + '    ' + extFlag + '    ' + data)

class MyUDPHandler(socketserver.BaseRequestHandler):

    def handle(self):
        data = self.request[0].strip()
        socket = self.request[1]
        print("{} wrote:".format(self.client_address[0]))
        print(data)
        socket.sendto(data.upper(), self.client_address)

if __name__ == "__main__":
    HOST, PORT = "192.168.1.3", 59581  # Local Network
 
    # Create the server, binding to localhost on port 9999
    with socketserver.UDPServer((HOST, PORT), MyUDPHandler) as server:
        #server.timeout(10)
        # Activate the server; this will keep running until you
        # interrupt the program with Ctrl-C
        server.serve_forever()

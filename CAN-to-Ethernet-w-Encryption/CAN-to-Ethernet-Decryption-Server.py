import socketserver
import Crypto as crypto
from Crypto.Cipher import AES

def decrypt(ciphertext, key):
    cipher = AES.new(key, AES.MODE_ECB)
    plaintext = cipher.decrypt(ciphertext)
    return plaintext

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

key = bytearray(
    [
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    ])

class MyTCPHandler(socketserver.BaseRequestHandler):

    def handle(self):
        pSize = 1456
        nFrames = 91
        # self.request is the TCP socket connected to the client

        self.data = self.request.recv(pSize)
        payload = [0]*pSize
        # This converts the encrypted data into humaan readable format, but does not decrypt
        for byte in range(0, pSize):
            payload[byte] = hex(self.data[byte])
        partialLoad = payload
        # The below code prints the encrypted data
        """
        for y in range(0, nFrames-1):
            pEncFrame = partialLoad[:16]
            partialLoad = partialLoad[16:] 
            frame = ''
            for val in range(0,16):
                dataVal = pEncFrame[val]
                dataVal = str(dataVal)[2:].upper()
                if len(dataVal) ==1:
                    dataVal = '0' + dataVal
                frame = frame + dataVal + ' '
            print(frame)
"""
        for y in range(0, nFrames - 1):
            pEncFrame = partialLoad[:16]
            partialLoad = partialLoad[16:]
            counter = 0
            bFrame = [0] * 16
            for item in pEncFrame:
                bFrame[counter] = int(item, 16)
                counter += 1
            bFrame = bytearray(bFrame)
            encFrame = [0] * 16
            counter = 0
            for item in bFrame:
                encFrame[counter] = hex(item)
                counter += 1
            #printFrame(encFrame) # Encrypted Frame Print
            ptFrame = decrypt(bFrame, key)
            decFrame = [0]*16
            counter = 0
            for item in ptFrame:
                decFrame[counter] = hex(item)
                counter += 1
            printFrame(decFrame) # Prints Decrypted Frame


if __name__ == "__main__":
    HOST, PORT = "192.168.1.83", 59581  # Local Network
    #HOST, PORT = "169.254.215.250", 59581  # Localhost, doesn't work


    # Create the server, binding to localhost on port 9999
    with socketserver.TCPServer((HOST, PORT), MyTCPHandler) as server:
        #server.timeout(10)
        # Activate the server; this will keep running until you
        # interrupt the program with Ctrl-C
        server.serve_forever()

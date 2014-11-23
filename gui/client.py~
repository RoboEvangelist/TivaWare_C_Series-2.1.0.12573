# File: server.py

import socket

#TCP_IP = '130.212.6.214'
TCP_IP = '192.168.1.110'
TCP_PORT = 5005
BUFFER_SIZE = 1024
MESSAGE = "HASC"

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print "connecting"
s.connect((TCP_IP, TCP_PORT))
print "connected"
#s.send(MESSAGE)
print "message sent"
while(1):
	try:
		print "waiting for data"
    		data = s.recv(BUFFER_SIZE)
		print "client get data: " + data
#		s.send(MESSAGE)
	except BaseException:
	#	print "error"
		pass
s.close()

print "received data:", data

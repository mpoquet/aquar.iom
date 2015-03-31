import socket
from sys import stdin

HOST = 'localhost'
PORT = 8080

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

def sendNTString(msg):
	try:
		sock.sendall(msg.encode('ascii') + b'\0')
	except socket.error as err:
		print('Error in send: {}'.format(err))

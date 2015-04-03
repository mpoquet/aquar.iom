import socket
from sys import stdin
import struct
from enum import Enum

IP = 'localhost'
PORT = 4242

class Stamp(Enum):
	LOGIN_PLAYER = 0
	LOGIN_VISU   = 1
	LOGIN_ACK    = 2      
	KICK         = 3
	LOGOUT       = 4
	WELCOME      = 5
	GAME_STARTS  = 6
	TURN         = 7
	TURN_ACK     = 8

class Client:
	def __init__(self, ip, port):
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect((ip, port))

	def sendUInt8(self, i):
		self.sock.sendall(struct.Struct('B').pack(*(i,)))

	def sendUInt32(self, i):
		self.sock.sendall(struct.Struct('<I').pack(*(i,)))

	def sendStamp(self, s):
		self.sendUInt8(s.value)

	def sendSizedString(self, msg):
		data = msg.encode('ascii')
		self.sendUInt32(len(data))
		self.sock.sendall(data)

	def sendLoginPlayer(self, nick):
		self.sendStamp(Stamp.LOGIN_PLAYER)
		self.sendSizedString(nick)

	def sendLoginVisu(self, nick):
		self.sendStamp(Stamp.LOGIN_VISU)
		self.sendSizedString(nick)


	def readUInt8(self):
		rawmsg = self.sock.recv(1)
		print('s=',rawmsg)
		(i,) = struct.Struct('B').unpack(rawmsg)
		print('read uint8:', i)
		return i

	def readUInt32(self):
		rawmsg = self.sock.recv(4)
		print('s=',rawmsg)
		(i,) = struct.Struct('<I').unpack(rawmsg)
		print('read uint32:', i)
		return i

	def readSizedString(self):
		length = self.readUInt8()
		s = str(self.sock.recv(length))
		print('read string:',s)
		return s


	def recvMessage(self):
		stamp = self.readUInt8()
		if stamp == Stamp.LOGIN_ACK.value:
			print('LOGIN_ACK')
		elif stamp == Stamp.KICK.value:
			reason = self.readSizedString()
			print('KICK:', reason)
		elif stamp == Stamp.LOGOUT.value:
			print('LOGOUT')
		else:
			print('unhandled stamp:', stamp)

c = Client(IP, PORT)

c.sendLoginPlayer('noob')
c.recvMessage()
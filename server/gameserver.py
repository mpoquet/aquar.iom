import asyncore
import socket
import re
from enum import Enum

ClientType = Enum('ClientType', 'unknown player visu lost')

nickValidator = re.compile('[\w _\-]+')
def isValidNick(nick):
    return nickValidator.fullmatch(nick) != None

class ClientHandler(asyncore.dispatcher_with_send):
    def __init__(self, sock=None, map=None):
        asyncore.dispatcher_with_send.__init__(self, sock, map)
        self.type = ClientType.unknown
        self.buffer = b''
        self.nick = 'Anonymous'
        self.longName = str(self.addr)

    def handle_read(self):
        data = self.recv(512)
        if not data:
            return

        self.buffer = self.buffer + data
        print("buffer of client {}: '{}'".format(self.longName, self.buffer))

        # UNLOGGED
        if self.type == ClientType.unknown:
            s = self.getString()
            if s[:5] == 'login':
                nick = s[5:].strip()
                if isValidNick(nick) and self.gameServer.addPlayer(self):
                    self.nick = nick
                    self.longName = "{} ({})".format(self.nick, self.id)
                    self.type = ClientType.player
                else:
                    self.kick("impossible to log in while the game is running")
            elif s[:4] == 'visu':
                nick = s[4:].strip()
                if isValidNick(nick) and self.gameServer.addVisu(self):
                    self.nick = nick
                    self.longName = "{} ({})".format(self.nick, self.id)
                    self.type = ClientType.visu
                else:
                    self.kick("impossible to log in while the game is running")
            elif s != '':
                self.kick("invalid string received ({})".format(s))

        # PLAYER
        elif self.type == ClientType.player:
            print("Received something from a player (buffer='{}')".format(self.buffer))
        
        # VISU
        elif self.type == ClientType.visu:
            print("Received something from a visu (buffer='{}')".format(self.buffer))

        if len(self.buffer) > 512:
            self.kick('buffer size exceeded')

    def handle_close(self):
        print('Client {} disconnected'.format(self.longName))
        self.close()
        self.onDisconnect()

    def kick(self, reason):
        self.sendString("kicked from server: " + reason)
        print("Client {} kicked: {}".format(self.longName, reason))
        self.close()
        self.onDisconnect()

    def onDisconnect(self):
        if self.type != ClientType.lost:    
            if self.type == ClientType.player:
                self.gameServer.removePlayer(self)
            elif self.type == ClientType.visu:
                self.gameServer.removeVisu(self)
            else:
                self.gameServer.removeUnloggedClient(self)
            self.type = ClientType.lost
            
    # Returns the string on the buffer if there is one, '' otherwise
    def getString(self):
        for i in range(len(self.buffer)):
            if chr(self.buffer[i]) == '\0':
                s = self.buffer[:i]
                self.buffer = self.buffer[i+1:]
                return s.decode()
        return ''

    # Sends a string (adds an ending null character)
    def sendString(self, s):
        try:
            self.send(s.encode('ascii') + b'\0')
        except socket.error as err:
            print("error: cannot send '{}' to client {}, reason='{}'".format(s, self.longName, err))

class GameServer(asyncore.dispatcher):
    def __init__(self, host, port):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.set_reuse_addr()
        self.bind((host, port))
        self.gameLaunched = False
        self.unknownClients = []
        self.players = []
        self.visus = []
        self.nextClientID = 0
        self.listen(5)

    def addPlayer(self, client):
        if not self.gameLaunched:
            print('New player: {} -> {}'.format(client.addr, client.nick))
            self.unloggedClients.remove(client)
            self.players.append(client)
            return True
        return False

    def addVisu(self, client):
        if not self.gameLaunched:
            print('New visu: {} -> {}'.format(client.addr, client.nick))
            self.unloggedClients.remove(client)
            self.visus.append(client)
            return True
        return False

    def removeUnloggedClient(self, client):
        self.unloggedClients.remove(client)

    def removePlayer(self, client):
        self.players.remove(client)

    def removeVisu(self, client):
        self.visus.remove(client)

    def handle_accept(self):
        pair = self.accept()
        if pair is not None:
            sock, addr = pair
            print('Incoming connection from {}'.format(addr))
            if not self.gameLaunched:
                client = ClientHandler(sock)
                client.gameServer = self
                client.id = self.nextClientID
                client.addr = addr

                self.nextClientID = self.nextClientID + 1
                self.unloggedClients.append(client)
            else:
                try:
                    print('Incoming connection closed (game already launched)')
                    sock.send(b'error: game launched\0')
                finally:
                    sock.close()


server = GameServer('localhost', 8080)
asyncore.loop()
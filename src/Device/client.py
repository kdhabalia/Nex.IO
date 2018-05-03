import socket

class Client:

  def __init__(self, sock=None):
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

  def connect(self, host, port):
    self.sock.connect((host, port))

  def send(self, msg):
    sent = self.sock.send(msg)

  def recv(self):
    BUFF_SIZE = 4096
    data = b''
    while True:
      chunk = self.sock.recv(BUFF_SIZE)
      data += chunk
      if len(chunk) < BUFF_SIZE:
        break
    return data

C = Client()
C.connect("128.2.13.145", 15742)





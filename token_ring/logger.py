#!/usr/bin/env python3

import datetime
import socket
import struct
import sys


filename = "log.txt"
if len(sys.argv) > 1:
    filename = sys.argv[1] 

file = open(filename, "a+")
data = []

LOGGER_IP = "224.0.0.42"
LOGGER_PORT = 1919

logger = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
logger.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

logger.bind((LOGGER_IP, LOGGER_PORT))

mreq = struct.pack("4sl", socket.inet_aton(LOGGER_IP), socket.INADDR_ANY)
logger.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

while True:
    data = logger.recv(1024)
    timestamp = str(datetime.datetime.now())
    print(timestamp + " : " + data.decode("utf-8"))
    file.write(timestamp + " : " + data.decode("utf-8"))

#!/usr/bin/env python3

import datetime
import socket
import struct
import sys


filename = "log.txt"
if len( sys.argv ) > 1:
    filename = sys.argv[1] 

file = open( filename, "a+" );
data = []

logger_ip = "224.0.0.42"
logger_port = 1919

logger = socket.socket( socket.AF_INET, socket.SOCK_DGRAM );
logger.bind( ( logger_ip, logger_port ) );

while True:
    data, addr = logger.recvfrom( 1024 )
    timestamp = str( datetime.datetime.now() )
    print( timestamp + " : " + data.decode( 'utf-8' ) )
    file.write( timestamp + " : " + data.decode( 'utf-8' ) )

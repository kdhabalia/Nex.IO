#!/bin/bash

gcc -Wall -std=gnu99 -lpthread -o capstoneServer/capstoneServer Queue/queue.c LoadBalancer/loadBalancer.c PyInterface/pythonInterface.c capstoneServer/csapp.c capstoneServer/sbuf.c capstoneServer/capstoneServer.c; ./capstoneServer/capstoneServer 15742 & python capstoneFileManager/makeFileDirectory.py & python capstoneFileManager/backendDataStruct.py &

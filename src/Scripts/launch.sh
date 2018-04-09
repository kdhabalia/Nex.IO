#!/bin/bash

gcc -Wall -std=gnu99 -lpthread -o ../Server/Server ../Queue/queue.c ../Scheduler/loadBalancer.c ../PyInterface/pythonInterface.c ../Server/csapp.c ../Server/sbuf.c ../Server/capstoneServer.c; ./../Server/Server 15743 & python ../FileManager/makeFileDirectory.py & python ../FileManager/backendDataStruct.py &

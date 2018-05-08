#!/bin/bash

(rm ../Server/Server; gcc -Wall -std=gnu99 -lpthread -o ../Server/Server ../Queue/queue.c ../Scheduler/loadBalancer.c ../PyInterface/pythonInterface.c ../Server/csapp.c ../Server/sbuf.c ../Server/capstoneServer.c) 2>&1 | grep --color -iP error

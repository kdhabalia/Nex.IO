#!/bin/bash

(./../Server/Server 15743 &) && (python ../FileManager/makeFileDirectory.py &) && (python ../FileManager/backendDataStruct.py &)

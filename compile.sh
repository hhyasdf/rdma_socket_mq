#!/bin/sh

g++ ./src/server.cpp ./include/*.cpp -o server -g -lrdmacm -libverbs -lpthread
g++ ./src/client.cpp ./include/*.cpp -o client -g -lrdmacm -libverbs -lpthread

#!/bin/sh

g++ ./test/server.cpp ./src/*.cpp -o server -g -lrdmacm -libverbs -lpthread
g++ ./test/client.cpp ./src/*.cpp -o client -g -lrdmacm -libverbs -lpthread

g++ ./test/socket_server.cpp ./src/*.cpp -o socket_server -g -lrdmacm -libverbs -lpthread
g++ ./test/socket_client.cpp ./src/*.cpp -o socket_client -g -lrdmacm -libverbs -lpthread

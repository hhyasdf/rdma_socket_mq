#!/bin/sh

gcc ./src/server.c ./include/*.c -o server -g -lrdmacm -libverbs -lpthread -std=gnu99
gcc ./src/client.c ./include/*.c -o client -g -lrdmacm -libverbs -lpthread -std=gnu99

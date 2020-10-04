CC=g++

FLAGS=-std=c++11 -o
DEBUG=-g

all: Nerf.cpp NerfPacket.h NerfPacket.cpp Utilities.h Utilities.cpp Server.h Server.cpp Client.h Client.cpp Measurements.h Measurements.cpp
	$(CC) $(FLAGS) nerf Nerf.cpp NerfPacket.cpp Utilities.cpp Server.cpp Client.cpp Measurements.cpp -lpthread

debug: Nerf.cpp NerfPacket.h NerfPacket.cpp Utilities.h Utilities.cpp Server.h Server.cpp Client.h Client.cpp Measurements.h Measurements.cpp
	$(CC) $(DEBUG) $(FLAGS) nerf Nerf.cpp NerfPacket.cpp Utilities.cpp Server.cpp Client.cpp Measurements.cpp -lpthread

clean: clear
clear:
	rm -rf nerf

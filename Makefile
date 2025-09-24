CXXFLAGS=-I $(HOME)/rapidjson
LDFLAGS=-lcurl
LD=g++
CC=g++

all: par_level_client

par_level_client: par_level_client.o
	$(LD) $< -o $@ $(LDFLAGS)

par_level_client.o: par_level_client.cpp
	$(CC) -c par_level_client.cpp $(CXXFLAGS)

clean:
	-rm par_level_client par_level_client.o
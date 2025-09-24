#CXXFLAGS=-I path/to/rapidjson
LDFLAGS=-lcurl
LD=g++
CC=g++

all: level_client par_level_client

level_client: level_client.o
	$(LD) $< -o $@ $(LDFLAGS)

par_level_client: par_level_client.o
	$(LD) $< -o $@ $(LDFLAGS)

par_level_client.o: par_level_client.cpp
	$(CC) -c par_level_client.cpp

clean:
	-rm level_client level_client.o

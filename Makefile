CXXFLAGS=-I$(HOME)/rapidjson/include -std=c++17
LDFLAGS=-lcurl
CC=g++
LD=g++

all: par_level_client

par_level_client: par_level_client.o
	$(LD) $< -o $@ $(LDFLAGS)

par_level_client.o: par_level_client.cpp
	$(CC) $(CXXFLAGS) -c par_level_client.cpp

clean:
	-rm -f par_level_client par_level_client.o
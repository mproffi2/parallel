CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS = -lcurl -lpthread

all: par_level_client

par_level_client: par_level_client.o
	$(CXX) $< -o $@ $(LDFLAGS)

par_level_client.o: par_level_client.cpp
	$(CXX) -c par_level_client.cpp $(CXXFLAGS)

clean:
	rm -f par_level_client par_level_client.o
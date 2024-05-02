
INCLUDES = -Iinclude/uWebSockets/src/ -Iinclude/uWebSockets/uSockets/src -Iinclude/easywsclient
LIBS = -Linclude/uWebSockets/uSockets/src -lz -lpthread
SOURCES = include/uWebSockets/uSockets/uSockets.a include/easywsclient/easywsclient.cpp
CXXFLAGS = -std=c++20

default:
	g++ $(INCLUDES) $(LIBS) $(SOURCES) $(CXXFLAGS) src/client.cpp -o build/client
	g++ $(INCLUDES) $(LIBS) $(SOURCES) $(CXXFLAGS) src/server.cpp -o build/server
	g++ $(CXXFLAGS) src/main.cpp -o build/main

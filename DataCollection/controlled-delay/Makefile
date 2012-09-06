source = socket.cc  hist.cc rate-estimate.cc payload.cc history.cc  codel-client.cc codel-server.cc delay-servo.cc
objects = socket.o hist.o rate-estimate.o payload.o history.o feedback.o delay-servo.o
executables = codel-client codel-server

CXX = g++
CXXFLAGS = -g -O3 -std=c++0x -ffast-math -pedantic -Werror -Wall -Wextra -Weffc++ -fno-default-inline -pipe -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE
LIBS = -lm -lrt

all: $(executables)

codel-client: codel-client.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

codel-server: codel-server.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include depend

depend: $(source)
	$(CXX) $(INCLUDES) -MM $(source) > depend

.PHONY: clean
clean:
	-rm -f $(executables) depend *.o *.rpo

source =  
objects = link.o  payload.o rate-schedule.o unrestrained-link.o token-bucket.o cbr-link.o schedule-link.o trace-link.o
executables = traffic-shaping udp-flood

CXX = g++
CXXFLAGS = -g -O3 -std=c++0x -ffast-math -pedantic -Werror -Wall -Wextra -Weffc++ -fno-default-inline -pipe -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE
LIBS = -lm -lrt -lfftw3 -lboost_program_options

all: $(executables)

traffic-shaping: traffic-shaping.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

udp-flood: udp-flood.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include depend

depend: $(source)
	$(CXX) $(INCLUDES) -MM $(source) > depend

.PHONY: clean
clean:
	-rm -f $(executables) depend *.o *.rpo

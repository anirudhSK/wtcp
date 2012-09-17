CC = gcc
CFLAGS := -Wall -g -ansi -std=c99
LDFLAGS = -Wl,--as-needed
LDADD := -lSDL
VIEWER_OBJECTS = sdlvideoviewer.o

.PHONY : clean distclean all
%.o : %.c
	$(CC) $(CFLAGS) -c $<

all: sdlvideoviewer

sdlvideoviewer: $(VIEWER_OBJECTS) 
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

clean:
	rm -f *.o

distclean : clean
	rm -f sdlvideoviewer


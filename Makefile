CC=gcc
INCLUDES= -I/usr/include/libxml2 -lxml2
CFLAGS= -g -ggdb $(INCLUDES)
DEPS = dmgParser.h apfs.h
OBJ = DMG.o base64.o apfs.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

make:	$(OBJ)
	$(CC) -o DMG $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o DMG

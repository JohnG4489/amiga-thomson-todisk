# Makefile for ToDisk

#### Start of system configuration section. ####
COMPILER=gcc
CFLAGS=-DNDEBUG -Wall -c -fPIC -fno-strict-aliasing
OFLAGS=-O3
DELETE=rm -f
STRIP=yes
STRIPOPT=--strip-debug
CINCLUDE=-I./
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
TARGET=todisk
#### End of system configuration section. ####

all: show $(OBJ)
ifeq ($(STRIP),yes)
	strip $(STRIPOPT) $(OBJ)
endif
	$(COMPILER) $(OBJ) -o $(TARGET)
	strip -s $(TARGET)
	ls -la $(TARGET)

show:
	@echo ""
	@echo "*******************"
	@echo "* Compile ToDisk/ *"
	@echo "*******************"

.c.o:
	$(COMPILER) $(CFLAGS) $(OFLAGS) $(CINCLUDE) $<

clean:
	@echo ""
	@echo "****************"
	@echo "* Clean ToDisk *"
	@echo "****************"
	$(DELETE) *~*
	$(DELETE) *.o
	$(DELETE) *.a

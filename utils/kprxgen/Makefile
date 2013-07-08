# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

CPP=gcc
CFLAGS=-I../common
LDFLAGS=
TARGET=psp-kprxgen
OBJECTS=psp-kprxgen.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Creating binary $(TARGET)"
	@$(CPP) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	@echo "Compiling $^"
	$(CPP) $(CFLAGS) -c $^ -o $@

clean:
	@echo "Removing all the .o files"
	@$(RM) $(OBJECTS)

mrproper: clean
	@echo "Removing binary"
	@$(RM) $(TARGET)


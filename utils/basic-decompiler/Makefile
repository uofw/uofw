# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

CPP=c++
CFLAGS=-Wall -Wextra -std=c++0x
LDFLAGS=
TARGET=dec
OBJECTS=main.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Creating binary $(TARGET)"
	$(CPP) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	@echo "Compiling $^"
	$(CPP) $(CFLAGS) -c $^ -o $@

clean:
	@echo "Removing all the .o files"
	@$(RM) $(OBJECTS)

mrproper: clean
	@echo "Removing binary"
	@$(RM) $(TARGET)

install: all
	@cp $(TARGET) ../bin


# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

CFLAGS=-Wall -Wextra -std=c++0x
LDFLAGS=
TARGET=basic-decompiler
OBJECTS=main.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Creating binary $(TARGET)"
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	@echo "Compiling $^"
	$(CXX) $(CFLAGS) -c $^ -o $@

clean:
	@echo "Removing all the .o files"
	@$(RM) $(OBJECTS)

mrproper: clean
	@echo "Removing binary"
	@$(RM) $(TARGET)

install: all
	@cp $(TARGET) ../bin


# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

CPP=gcc
CFLAGS=-Wall -Wextra -Werror
LDFLAGS=
TARGET=psp-build-exports
OBJECTS= ..\common\sha1.o psp-build-exports.o

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


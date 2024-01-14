# Makefile
# Author: Pato Silva
# Put it on src folder

# Include dir
IDIR =../include

# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -Wall -I$(IDIR)

# Program name
TARGET = fatfstest

# Files with no headers, like main.c
SRCS = main.c

# Dependency files
_DEPS = md5.h ciphdev.h speck.h ff.h diskio.h
_OBJ = $(_DEPS:.h=.o)

# Directories
ODIR=obj
LDIR =../lib

# Libs to Include
LIBS =

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(TARGET): $(OBJ) $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(ODIR)/%.o: %.c $(ODIR) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $(ODIR)

.PHONY: clean

clean:
	$(RM) $(ODIR)/*.o $(TARGET)
	$(RM) -d $(ODIR)

run: $(TARGET)
	./$(TARGET)

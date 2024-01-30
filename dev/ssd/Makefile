
# Makefile
# Author: Pato Silva
# Put it on src folder

# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -Wall

# Program name
TARGET = ssd

# Files with no headers, like main.c
SRCS = ssd.c

# Dependency files
DEPS = fonts.h mytimer.h ssd1306_i2c.h
_OBJ = $(DEPS:.h=.o)

# Directories
ODIR=obj

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(TARGET): $(OBJ) $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

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

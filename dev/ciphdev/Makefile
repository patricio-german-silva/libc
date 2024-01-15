# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -Wall -Wpedantic

# the build target executable:
TARGET = main

# define the C source files
SRCS = main.c ciphdev.c md5.c speck_sc.c

# define the C object files
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(OBJS):
	$(CC) $(CFLAGS) -c $(SRCS)

$(TARGET): $(OBJS) $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	$(RM) $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET)

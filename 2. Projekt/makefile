CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
SRCS = proj2.c
OBJS = $(SRCS:.c=.o)
EXEC = proj2

# Default target
all: $(EXEC)

# Rule to link object files into the executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove object files and the executable
clean:
	rm -f $(OBJS) $(EXEC)

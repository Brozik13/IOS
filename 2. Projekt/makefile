CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
SRCS = proj2.c
OBJS = $(SRCS:.c=.o)
EXEC = proj2

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
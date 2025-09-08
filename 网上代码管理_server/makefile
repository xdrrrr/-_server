CFLAGS=-I./inc
LBFLAGS=-L./ -lpthread -lcjson -lsqlite3

app=server
CC=gcc

SRC=$(wildcard ./src/*.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

$(app):$(OBJS)
	$(CC) $^ -o $(app) $(LBFLAGS)
.phony:clean
clean:
	rm -r $(OBJS) $(app)

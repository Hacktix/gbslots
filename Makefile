AS = lcc -c
CC = lcc -Wa-l -Wl-m

BIN = main.gb
OBJS = main.o

all: $(BIN)

%.s: %.ms
	maccer -o $@ $<

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS)

clean:
	rm -rf $(BIN) $(OBJS) *~
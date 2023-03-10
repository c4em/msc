CC = cc
CFLAGS = -Os -flto -Wall -Wextra
DCFLAGS = -Og -g -Wall -Wextra
LDFLAGS = `pkg-config --libs md4c-html`

SRC = msc.c
OUT = msc

options:
	@echo "build options:"
	@echo "-------------------"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"
	@echo "-------------------"

msc: options
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(OUT)

debug:
	$(CC) $(SRC) $(DCFLAGS) $(LDFLAGS) -o $(OUT)-d
	$(CC) $(SRC) $(DCFLAGS) $(LDFLAGS) -fsanitize=address -o $(OUT)-ds

clean:
	rm -rf $(OUT) $(OUT)-d $(OUT)-ds dist/

.DEFAULT_GOAL := msc


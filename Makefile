CC = cc
CFLAGS = -Os -flto -Wall -Wextra

SRC = msc.c
OUT = msc

options:
	@echo "build options:"
	@echo "-------------------"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "CC     = $(CC)"
	@echo "-------------------"

msc: options
	$(CC) $(SRC) $(CFLAGS) -o $(OUT)

debug:
	$(CC) $(SRC) $(CFLAGS) -o $(OUT)-d
	$(CC) $(SRC) $(CFLAGS) -fsanitize=address -o $(OUT)-ds

clean:
	rm -rf $(OUT) $(OUT)-d $(OUT)-ds dist/

.DEFAULT_GOAL := msc


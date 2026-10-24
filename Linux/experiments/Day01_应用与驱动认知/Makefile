CC      ?= cc
CFLAGS  ?= -Wall -Wextra -std=c11 -O0 -g
TARGET  := hello_arg

.PHONY: all run clean

all: $(TARGET)

$(TARGET): hello_arg.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	./$(TARGET)
	./$(TARGET) Linux
	./$(TARGET) Linux driver

clean:
	rm -f $(TARGET) $(TARGET).exe

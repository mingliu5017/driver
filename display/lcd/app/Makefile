#
## sample Makefile for helloworld
#
#
OBJ = lcd_display.o
CFLAGS = -Wall -Wextra
TARGET=lcd_display
LIB = -lpthread -lm -lrt

# rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LIB)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f $(OBJ)

install:
	cp lcd_display $(DESTDIR)/bin/lcd_display

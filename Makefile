CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -framework CoreGraphics -framework ApplicationServices -lpthread

TARGET = model_o_autoclicker

all: $(TARGET)

$(TARGET): model_o_autoclicker.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET)

install: all
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean install
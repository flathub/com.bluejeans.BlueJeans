DEBUG=0
CFLAGS=-g -Wall -DDEBUG=$(DEBUG)

all: bluejeans-single-app

clean:
	rm -f bluejeans-single-app

bluejeans-single-app: bluejeans-single-app.c Makefile
	gcc $(CFLAGS) -o $@ `pkg-config --libs --cflags gio-2.0` $<

install: bluejeans-single-app
	install -Dm 755 bluejeans-single-app /app/bin/bluejeans

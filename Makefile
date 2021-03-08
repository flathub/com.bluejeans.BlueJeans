DEBUG=0
CFLAGS=-g -Wall -DDEBUG=$(DEBUG)

all: bluejeans-single-app libmock-udev.so

clean:
	rm -f bluejeans-single-app libmock-udev.so mock-udev.o

bluejeans-single-app: bluejeans-single-app.c Makefile
	gcc $(CFLAGS) -o $@ `pkg-config --libs --cflags gio-2.0` $<

mock-udev.o: mock-udev.c Makefile
	gcc -g -Wall -DDEBUG=$(DEBUG) -fPIC -c -o mock-udev.o mock-udev.c

libmock-udev.so: mock-udev.o
	gcc -g -shared -fPIC -o libmock-udev.so mock-udev.o

install: bluejeans-single-app
	install -Dm 755 bluejeans-single-app /app/bin/bluejeans
	install -m755 -Dt /app/lib libmock-udev.so

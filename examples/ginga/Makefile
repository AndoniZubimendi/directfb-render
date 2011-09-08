all: test-ginga

test-ginga: test-ginga.c 
	gcc -o test-ginga test-ginga.c -lX11 -lXext -lXrender -ggdb -Wall

clean:
	rm -rf test-ginga

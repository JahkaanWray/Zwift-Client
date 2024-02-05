all: main

main: main.c
	gcc -o main main.c -g -lsetupApi -lbluetoothApis

clean:
	rm -rf main
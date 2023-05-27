CC = g++
CCFLAGS = -Wall -Wextra -std=c++17 -O0 -lm

.PHONY: build clean

build: client

client: client.o http_buffer.o helpers.o
	$(CC) -o $@ $^ $(CCFLAGS)

client.o: client.cpp
	$(CC) -c -o $@ $< $(CPPFLAGS)

http_buffer.o: http_buffer.cpp http_buffer.h
	$(CC) -c -o $@ $< $(CPPFLAGS)

helpers.o: helpers.cpp helpers.h
	$(CC) -c -o $@ $< $(CPPFLAGS)

clean:
	rm *.o client

zip:
	zip -u Popa_Rares_Teodor_324CB_Tema3PC.zip *.cpp *.h Makefile README
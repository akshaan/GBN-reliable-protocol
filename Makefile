all:
	g++  server.c -o server
	g++   client.c -o client

clean:
	rm -f server
	rm -f client

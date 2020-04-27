all: hellor

hellor: client.c server.c relay.c
	gcc -o client client.c; gcc -o server server.c; gcc -o relay relay.c
clean:
	rm client server relay
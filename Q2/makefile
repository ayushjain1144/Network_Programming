all: hellor

hellor: selective_repeat_client.c selective_repeat_server.c relay.c
	gcc -o client selective_repeat_client.c; gcc -o server selective_repeat_server.c; gcc -o relay relay.c
clean:
	rm client server relay
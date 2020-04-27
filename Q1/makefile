all: hellor

hellor: stop_and_wait_client.c stop_and_wait_server.c
	gcc -o client stop_and_wait_client.c; gcc -o server stop_and_wait_server.c
clean:
	rm client server
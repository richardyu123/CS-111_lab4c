.SILENT:

default: tcp tls

tcp:
	gcc -lm -lmraa -o lab4c_tcp -g lab4c_tcp.c

tls:

clean:
	rm lab4c_tcp

.SILENT:

default:
	gcc -lm -lmraa -o lab4c_tcp -g lab4c_tcp.c

clean:
	rm lab4c_tcp

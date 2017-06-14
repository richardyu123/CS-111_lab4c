.SILENT:

FILES=lab4c_tcp.c lab4c_tls.c README Makefile

default: tcp tls

tcp:
	gcc -lm -lmraa -o lab4c_tcp -g lab4c_tcp.c

tls:
	gcc -lm -lmraa -lssl -lcrypto -o lab4c_tls -g lab4c_tls.c

clean:
	rm lab4c_tcp lab4c_tls

dist:
	tar -zcvf lab4c-304464688.tar.gz $(FILES)

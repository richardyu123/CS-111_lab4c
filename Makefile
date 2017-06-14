.SILENT:

FILES=lab4c_tcp.c lab4c_tls.c README Makefile

default: tcp tls

tcp:
	gcc -lm -lmraa -o lab4c_tcp -g lab4c_tcp.c

tls:
	gcc -lm -lmraa -lssl -lcrypto -o lab4c_tls -g lab4c_tls.c

clean:
	rm -f lab4c_tcp lab4c_tls lab4c-304464688.tar.gz

dist:
	tar -zcvf lab4c-304464688.tar.gz $(FILES)

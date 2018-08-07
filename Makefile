.PHONY: all cgi
all: http_server

http_server: http_server.c
	gcc -o $@ $^ -g

.PHONY: cgi
cgi:
	cd cgi; make;cd -

clean:
	rm -f http_server
	cd cgi; make clean;cd -
	

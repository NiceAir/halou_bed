.PHONY: all cgi
all: http_server cgi

http_server: http_server.c
	gcc -o $@ $^ -g

#.PHONY: cgi
cgi:
	cd cgi/sql_connect; make;cd -

clean:
	rm -f http_server
	cd cgi; make clean;cd -
	

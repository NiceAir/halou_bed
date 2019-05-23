.PHONY: all cgi 
all: http_server cgi filemode 

http_server: http_server.c
	gcc -o $@ $^ -g

#.PHONY: cgi
cgi:
	cd cgi/sql_connect; make;cd -

clean:
	rm -f http_server
	cd cgi; make clean;cd -

filemode:
	cd wwwroot/; pwd; chmod a-x *.html;cd - 
	cd wwwroot/css/; pwd; chmod a-x *.css;cd -
	cd wwwroot/js; pwd; chmod a-x *.js;cd -;
	cd wwwroot/images; pwd; chmod a-x *;cd -;
	cd wwwroot/log_about; pwd; chmod a-x *.html;cd -;
	

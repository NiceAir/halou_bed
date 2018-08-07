cc=g++
object=math_cgi test

.PHONY: all sql_connect
all: $(object)

math_cgi: math_cgi.cpp
	$(cc) -o $@ $^

test: test.c
	gcc -o $@ $^

.PHONY: sql_connect
sql_connect:
	cd sql_connect; make; cd -

.PHONY: clean
clean:
	rm -f $(object)
	cd sql_connect; make clean; cd -

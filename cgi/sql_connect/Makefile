
CURR_PATH=$(shell pwd)
LIB=$(CURR_PATH)/../../lib
INCLUDE=$(LIB)/include
MYSQL_LIB=$(LIB)/lib -lmysqlclient -lssl -lcrypto
.PHONY: all
all: save_img get_id get_urls user_save_img

save_img: save_img.c
	gcc -g -o $@ $^ -I$(INCLUDE) -L$(MYSQL_LIB)  
get_id: get_id.c
	gcc -g -o $@ $^ -I$(INCLUDE) -L$(MYSQL_LIB)  
get_urls: get_urls.c
	gcc -g -o $@ $^ -I$(INCLUDE) -L$(MYSQL_LIB)  
user_save_img: user_save_img.c
	gcc -g -o $@ $^ -I$(INCLUDE) -L$(MYSQL_LIB)  


.PHONY: clean
clean:
	rm -f save_img get_id

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<ctype.h>
#include<errno.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#define PORT 9999
#define IP "120.79.49.217"
#define MAX_EVENTS 30
#define MAX_COLS 4096
#define HOME_PAGE "index.html"
#define PAGE_400 "wwwroot/400.html"
#define PAGE_404 "wwwroot/404.html"
#define PAGE_500 "wwwroot/500.html"
#define PAGE_201 "wwwroot/201.html"
char response_head[][40] = {
						{"200 OK\r\n"},         			// 0
						{"400 Bad Request\r\n"},			// 1
						{"404 Not Found\r\n"},  			// 2
						{"500 Internal Server Error\r\n"}}; // 3
char content_type[][30] = {
							{"text/html"},					/*html*/
							{"text/css"},					/*css*/
							{"application/x-javascript"}, 	/*js*/
							{"image/gif"},					/*gif*/
							{"image/jpeg"},					/*jpg*/
							{"image/png"}};					/*png*/
//**************************************哈希表*************************************
typedef struct head_env{
	int fd;  //状态，负1表示这结构还没有使用
	int imgid; 
	int status_code;
	int clos;	//为1表示这个链接已失效
	char method[MAX_COLS/32];
	char url[MAX_COLS/8];
	char query_string[MAX_COLS];
}head_env;

typedef struct haxi{
	head_env *arry;
	int capacity;
}haxi;

void haxiInit(haxi *p, int size)
{
	head_env *tmp = (head_env *)malloc(sizeof(head_env)*size);
	if(tmp == NULL)
	{
		perror("haxiInit");
		exit(1);
	}
	int i = 0;
	for(i = 0; i<size; i++)
	{
		tmp[i].fd = -1;
		memset(tmp[i].method, 0x00, sizeof(tmp[i].method));	
		memset(tmp[i].url, 0x00, sizeof(tmp[i].url));	
		memset(tmp[i].query_string, 0x00, sizeof(tmp[i].query_string));	
		tmp[i].imgid = -1;
		tmp[i].status_code = 200;
		tmp[i].clos = 1;
	}
	p->arry = tmp;
	p->capacity = size;
}

int  haxiInsert(haxi *p, int fd, char *method, char *url, char *query_string, int id, int status_code, int clos)
{
	if(fd > (p->capacity-1))
	{
		//enalrge size;
		head_env *tmp = (head_env *)realloc(p->arry, sizeof(head_env)*fd*2);
		if(tmp == NULL)
			return -1;
		int i = p->capacity;
		for(i = p->capacity; i<fd*2; i++)
		{
			tmp[i].fd = -1;
			memset(tmp[i].method, 0x00, sizeof(tmp[i].method));	
			memset(tmp[i].url, 0x00, sizeof(tmp[i].url));	
			memset(tmp[i].query_string, 0x00, sizeof(tmp[i].query_string));	
			tmp[i].imgid = -1;
			tmp[i].status_code = 200;
			tmp[i].clos = 1;
		}
		p->arry = tmp;
		p->capacity = fd*2;
	}
	p->arry[fd].fd = fd;
	strcpy(p->arry[fd].method, method);
	strcpy(p->arry[fd].url, url);
	if(query_string)
		strcpy(p->arry[fd].query_string, query_string);
	p->arry[fd].imgid = id;
	p->arry[fd].status_code = status_code;
	p->arry[fd].clos = clos;
	printf("设置参数：method: %s    url: %s    query_string: %s    id: %d    status_code: %d    sock:%d    close:%d\n", method, url, query_string, id, status_code, fd, clos);
	return 0;
}

int haxiDelete(haxi *p, int fd)
{
	if(p->arry[fd].fd == -1 || fd >= p->capacity)
		return -1;
	p->arry[fd].fd = -1;
	memset(p->arry[fd].method, 0x00, sizeof(p->arry[fd].method));	
	memset(p->arry[fd].url, 0x00, sizeof(p->arry[fd].url));	
	memset(p->arry[fd].query_string, 0x00, sizeof(p->arry[fd].query_string));	
	p->arry[fd].imgid = -1;
	p->arry[fd].status_code = 200;
	p->arry[fd].clos = 1;
	return 0;
}

void haxiDestory(haxi *p)
{
	free(p->arry);
}

haxi head_haxi;  //定义全局变量
int imgid = 0;
//*********************************************************************************
void show_time()
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL) < 0)
		return;
	printf("second=%ld,		usecond=%ld\n", (long)tv.tv_sec, (long)tv.tv_usec);
}

int set_noblock(int fd)
{
	int f = fcntl(fd, F_GETFL, 0);
	if(f < 0)
		return -1;
	if(fcntl(fd, F_SETFL, f | O_NONBLOCK) < 0)

		return -1;
	return 0;
}

void handler_accept(int epfd, int listen_sock)
{
	while(1)
	{
		struct sockaddr_in server;
		unsigned int len = sizeof(server);
		int server_sock = accept(listen_sock,(struct sockaddr *)&server, &len);
		if(server_sock < 0)
		{
			if(errno == EAGAIN)
				break;
			else
				continue;
		}
		else
		{
			struct epoll_event eve;
			eve.events = EPOLLIN | EPOLLET;
			eve.data.fd = server_sock;
			set_noblock(server_sock);
			if(epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &eve) == 0)
			{
				printf("新的链接已建立, 来自%s:%d, **************************sock:%d************************\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port), server_sock);
			}
			else
			{
				printf("链接建立失败*******************************sock:%d************************************\n", server_sock);
				close(server_sock);
			}
		}

	}
}
//\n \r\n \r
int get_line(int sock, char buf[], int cols, int *clo)
{
	int count = 0;
	char ch = '1';
	while(ch != '\n' && count < cols - 1)
	{
		if(recv(sock, &ch, 1, 0) == 0)
		{
			*clo = 1;
			break;
		}
		if(ch == '\r')
		{
			recv(sock, &ch, 1, MSG_PEEK);
			if(ch == '\n')
				recv(sock, &ch, 1, 0);
			else
				ch = '\n';		
		}
		buf[count++] = ch;
	}
	buf[count] = '\0';
	printf("%s", buf);
	return count;
}
void clear_header(int sock)
{
	char line[MAX_COLS];
	int clos;
	do
	{
		get_line(sock, line, MAX_COLS, &clos);
	}while(strcmp(line, "\n") != 0);
}

int saveImg(int sock, char *length, char *url, int urllen, char *type)
{
	int status = 200;
	int output[2];
	pipe(output);
	pid_t pid = fork();
	if(pid < 0)
		return 500;
	if(pid == 0)
	{
		char s[100] = {0};
		char *boundary = NULL;
		strtok(type, "=");
		boundary = strtok(NULL, "=");
		sprintf(s, "sock=%d&length=%s&boundary=%s", sock, length, boundary);
		printf("```````````````s:%s``````````````\n", s);
		close(output[0]);
		dup2(output[1], 1);    //将标准输出重定向为写管道。进程映像替换之后printf的内容就写到了写管道当中
		execl("cgi/sql_connect/save_img", "cgi/sql_connect/save_img", s, NULL);
		perror("execl失败");
		exit(500);
	}
	close(output[1]);
	memset(url, 0x00, urllen);
	char c;
	wait(NULL);
	ssize_t s = read(output[0], url, urllen);
	url[s] = 0;
	int i = 0;
	int code = 200;
	for(i = 0; i<strlen(url); i++)
	{
		if(url[i] == '&')
		{
			url[i] = '\0';
			code = atoi(url+i+1);
			break;
		}
	}
	
	if(code == 500)
		status = 500;
	close(output[0]);
	printf("获得cgi处理结果：url= %s  code= %d\n", url, code);
	return status;
}

//需要往传出参数query_by_cgi里写的是图片的url、用户名
//存储是否成功以该函数的返回值为准
int userSave(int sock, char *content_length, char *query_by_cgi, int query_len, char *type)
{
 	int status = 500;
	int output[2];
	pipe(output);
	pid_t pid = fork();
	if(pid < 0)
		return 500;
	if(pid == 0)
	{
		char s[200] = {0};
		char *boundary = NULL;
		strtok(type, "=");
		boundary = strtok(NULL, "=");
		sprintf(s, "sock=%d&length=%s&boundary=%s", sock, content_length, boundary);
		printf("```````````````登录后上传图片s:%s``````````````\n", s);
		close(output[0]);
		dup2(output[1], 1);   
		execl("cgi/sql_connect/user_save_img", "cgi/sql_connect/user_save_img", s, NULL);
		perror("execl失败");
		exit(500);
	}
	close(output[1]);
	wait(NULL);
	char tmp[300] = {0};
	ssize_t s = read(output[0], tmp, sizeof(tmp));
	tmp[s] = 0;
	close(output[0]);
	status = atoi(tmp);
	printf("tmp:%s\n", tmp);
	if(status == 200)
		sprintf(query_by_cgi, "%s", tmp+4);
	printf("cgi执行完毕\n");
	return status;

}


/*
s: 缓存区 slen：缓存区的大小
length: 需要读取的字节数

 */
void readbytes(int sock, char *s,int slen, int length)
{
	int count = 0;
	while(count < length)
	{
		char c;
		ssize_t ss = read(sock, &c, 1);
		if(ss < 0)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
		}
		else
		{
			s[count] = c;
			count++;
		}
		if(count >= slen)
			break;
	}
}

/*
  执行结果有6种可能：
  参数数量错误、两次密码不一致、&400  //通过错误处理进行响应
  链接数据库失败&500                  //通过错误处理进行响应 
  注册失败&500                        //通过错误处理进行响应
  该用户已存在&200
  注册成功&200
 */
int userRegister(int sock, char *content_length, char *url, int urllen)
{
	int status = 200;
	int output[2];
	
	pipe(output);
	pid_t pid = fork();
	if(pid < 0)
	{
		return 500;
	}
	if(pid == 0) //子进程
	{
		char s[200] = {0};
		readbytes(sock, s, 200, atoi(content_length));
		printf("s is %s\n", s);
		close(output[0]);
		dup2(output[1], 1);
		chdir("cgi/sql_connect");
    	execl("sh/register.sh", "sh/register.sh" , s, NULL);
		perror("execl 失败");
		exit(500);
	}
	close(output[1]);
	memset(url, 0x00, urllen);
	char c;
	wait(NULL);
	ssize_t s = read(output[0], url, urllen);
	url[s] = 0;
	int i = 0;
	int code = 200;
	for(i = 0; i<strlen(url); i++)
	{
		if(url[i] == '&')
		{
			url[i] = '\0';
			code = atoi(url+i+1);
			break;
		}
	}
	
	if(code == 500)
		status = 500;
	close(output[0]);
	printf("获得cgi处理结果：url= %s  code= %d\n", url, code);
	return status;
}

int getId()
{
	int id = -1;
	int output[2];
	pipe(output);
	pid_t pid = fork();
	if(pid < 0)
		return 0;
	if(pid == 0)
	{
		close(output[0]);
		dup2(output[1], 1);
		execl("/home/ly/study/Linux/net/http/cgi/sql_connect/get_id", "/home/ly/study/Linux/net/http/cgi/sql_connect/get_id", NULL);
		exit(1);
	}
	else 
	{
		close(output[1]);
		char tmp[11] = {0};
		ssize_t s = read(output[0], tmp, sizeof(tmp));
		tmp[s] = 0;
		id = atoi(tmp);
		wait(NULL);
		close(output[0]);
	}
	if(id < 0)
	{	
		printf("初始化失败\n");
		exit(1);
	}
	return id;
}

//返回值表登录成功与否，若登录成功，则query_by_cgi中是cooike
int userLogin(int sock, char * content_length, char *query_by_cgi, int query_len) //登录结果只有两种：登录成功和登录失败
{
	int output[2];
	printf("userLogin获得参数，sock:%d, content_length:%s, query_by_cgi:%s\n", sock, content_length, query_by_cgi);
	pipe(output);
	pid_t pid = fork();
	if(pid < 0)
		return 0;
	if(pid == 0)
	{
		char s[300] = {0};
		char buf[1024] = {0};
		readbytes(sock, s, sizeof(s), atoi(content_length));
		printf("s:%s\n", s);
		close(output[0]);
		dup2(output[1], 1);
		chdir("cgi/sql_connect");
    	execl("sh/login.sh", "sh/login.sh" , s, NULL);
		perror("ececl 失败");
		exit(1);
	}
	close(output[1]);
	wait(NULL);
	char tmp[400] = {0};   //临时接收cgi的返回结果，其中状态码为3位,cooike为32位
	size_t s = read(output[0], tmp, sizeof(tmp));
	tmp[s] = 0;
	int status = atoi(tmp);
	sprintf(query_by_cgi, "%s", tmp+4);
	printf("userLogin中CGI的执行结果: status=%d   %s\n", status, query_by_cgi);
	close(output[0]);
	return status;
}

void handler_request(int epfd, int sock)
{
	char method[MAX_COLS/32] = {0}; //请求方法字段
	char url[MAX_COLS/8] = {0}; //url字段
	char *query_string = NULL; //参数字段
	int status_code = 200;
	int id = -1;
	char query_by_cgi[1024] = {0};  //cgi返回的结果

	char line[MAX_COLS] = {0};
	char content_length[MAX_COLS/16] = {0}; //正文长度
	char content_buf[4096];	
	int clos = 0;
	int s = get_line(sock, line, sizeof(line), &clos);
	int i = 0;
	int j = 0;
	line[s] = 0;
	if(clos)
		goto set;
	while(i<sizeof(method) && !isspace(line[j]))
	{
		method[i++] = line[j++];
	}
	method[i] = '\0';
	while(isspace(line[j])) j++;
	i = 0;
	while(i<sizeof(url) && !isspace(line[j]))
	{
		url[i++] = line[j++];
	}
	url[i] = '\0';
	if(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)
	{
		for(i = 0; i<strlen(url); i++)
		{
			if(url[i] == '?')
			{
				url[i] = '\0';
				query_string = url+i+1;
				break;
			}
		}
		clear_header(sock);
		printf("method: %s    url: %s    query_string: %s    id: %d    status_code: %d\n", method, url, query_string, id, status_code);
	}
	else if(strcasecmp(method, "POST") == 0)
	{
		//post方法，取head中的正文长度字段
	//	get_line(sock, line, sizeof(line));    //host
		printf("method: %s    url: %s    query_string: %s    id: %d    status_code: %d\n", method, url, query_string, id, status_code);
		char type[200] = {0};
		int sign = 2;
		int cl = 0;
		do
		{
			memset(line, 0x00, sizeof(line));
			get_line(sock, line, sizeof(line), &cl);
			if(strncasecmp(line, "content-length: ", 16) == 0)
			{
				sign--;
				int i = 0;
				int j = 0;
				for(i = 16; i<strlen(line)-1; i++)
					content_length[j++] = line[i];
				content_length[j] = 0;
				if(sign == 0)
					break;
			}
			if(strncasecmp(line, "content-type: ", 14) == 0)
			{
				sign--;
				int i = 0;
				int j = 0;
				for(i = 14; i<strlen(line)-1; i++)
					type[j++] = line[i];
				type[j] = 0;
				if(sign == 0)
					break;
			}
		}while(strcmp(line, "\n") != 0);
		if(strlen(content_length) == 0)
		{
			status_code = 400;
			goto set;
		}
		clear_header(sock);
		if(strcmp(url, "/save") == 0)  //存图片
		{
			//post 接收正文部分中图片的数据并存入数据库，同时把imgid拿到
			printf("存图片，sock=%d, url=%s, content_length=%s, type=%s\n", sock, url, content_length, type);
			memset(query_by_cgi, 0x00, sizeof(query_by_cgi));
			saveImg(sock, content_length, query_by_cgi, sizeof(query_by_cgi), type);
			if(strlen(query_by_cgi) == 7)
			{
				status_code = 500;
				printf("存图片cgi的结果为空\n");
				goto set;
			}
			printf("图片已存入, 此时query_by_cgi=%s\n", query_by_cgi);
			id = imgid++;
		}
		else if(strcmp(url, "/log_about/register") == 0) //注册
		{
			printf("进行注册，sock=%d, url=%s, content_length=%s, type=%s\n", sock, url, content_length, type);
			memset(query_by_cgi, 0x00, sizeof(query_by_cgi));
			status_code =  userRegister(sock, content_length, query_by_cgi, sizeof(query_by_cgi));	
			if(strlen(query_by_cgi) == 0)
			{
				status_code = 500;
				printf("注册的cgi结果为空\n");
				goto set;
			}
			printf("用户注册成功, 此时query_by_cgi=%s\n", query_by_cgi);

		}
		else if(strcmp(url, "/log_about/login") == 0) //登录
		{
			printf("用户登录，sock=%d, url=%s, content_length=%s, type=%s\n", sock, url, content_length, type);
			status_code = userLogin(sock, content_length, query_by_cgi, sizeof(query_by_cgi)); //登录结果只有两种：登录成功和登录失败
		}
		else if(strcmp(url, "/log_about/save") == 0)
		{
			printf("登录后上传图片，sock=%d, url=%s, content_length=%s, type=%s\n", sock, url, content_length, type);
			status_code = userSave(sock, content_length, query_by_cgi, sizeof(query_by_cgi), type);
		}
		else   //POST 的url未知，返回404页面
		{
			printf("其它方式\n\n");	
			status_code = 400;
		}
	}
	else //other method
	{
		printf("其它方法\n\n");
	}

set:	
	if(strcasecmp(method, "POST") == 0)
	{
		query_string = query_by_cgi;
	}
	if(haxiInsert(&head_haxi, sock, method, url, query_string, id, status_code, clos) < 0)
	{
		epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL);
		close(sock);
		printf("sock为%d的链接导入参数失败\n", sock);
		return;
	}
	//show_time();
	printf("导入参数：method: %s    url: %s    query_string: %s    id: %d    status_code: %d    sock:%d    close:%d\n", method, url, query_string, id, status_code, sock, clos);
	struct epoll_event eve;
	eve.events = EPOLLOUT | EPOLLET;
	eve.data.fd = sock;
	epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &eve);
}

void type_change(char **type)
{
	if(strcasecmp(*type, "css") == 0)
		*type = content_type[1];

	else if(strcasecmp(*type, "js") == 0)
		*type = content_type[2];

	else if(strcasecmp(*type, "gif") == 0)
		*type = content_type[3];

	else if(strcasecmp(*type, "jpg") == 0)
		*type = content_type[4];

	else if(strcasecmp(*type, "png") == 0)
		*type = content_type[5];
	else 
		*type = content_type[0];
}

void echo_head(int sock, char *head, char *type, char *path)
{
	char line[1024] = {0};

	sprintf(line, "HTTP/1.0 %s", head);
	printf("%s\n", line);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	type = strrchr(path, '.');		
	type++;
	type_change(&type);
	
	printf("path=%s		type=%s\n", path, type);
	sprintf(line, "Content-Type: %s;charset=IOS-8859-1\r\n", type);
	printf("%s\n", line);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	int len = 0;
	struct stat st;
	if(stat(path, &st) == 0)
	{
		len = st.st_size;
	}
	sprintf(line, "Content-Length: %d\r\n", len);
	printf("%s\n", line);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	sprintf(line, "\r\n");
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));
	
}

int echo_www(int sock, char *path, int status_code)
{
	char *head = NULL;
	char *type = NULL;
	char line[4096] = {0};
	if(status_code == 200)
		head = response_head[0];
	else if(status_code == 400)
		head = response_head[1];
	else if(status_code == 404)
		head = response_head[2];
	else
		head = response_head[3];
	
	printf("echo_www---path:%s\n", path);

	echo_head(sock, head, type, path);
	
	int fd = open(path, O_RDONLY);
	if(fd < 0)
		return 500;

	struct stat st;
	if(stat(path, &st) == 0)
	{
//		sendfile(sock, fd, NULL, st.st_size);
		size_t num = 0;       //已发送的字符数
		size_t need = st.st_size;   //还需发送的字符数
		printf("start sendfile sock=%d  fd=%d  need=%d\n", sock, fd, (int)need);
		while(need > 0 )
		{

			ssize_t send_num = sendfile(sock, fd, &num, need);
			if(send_num < 0 )
			{
				if(errno == EAGAIN || errno == ENOMEM)
					continue;
				else
					break;
			}
			need -= send_num;
		}
		printf("end sendfile sock=%d  fd=%d  num=%d need=%d\n", sock, fd, num, (int)need);
	}
	else
	{
		printf("文件%s发送失败\n", path);
	}
	close(fd);
	return 200;
}

void echo_www_url(int sock, char *path)
{
	char line[4096];
	char source[50] = {0};
	printf("path: %s\n", path);
	sprintf(source, "http://%s:%d/%s", IP, PORT, path);
	printf("path: %s\n", path);
	printf("source: %s\n", source);
	int source_len = strlen(source);
	sprintf(line, "HTTP/1.0 200 OK\r\n");
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	sprintf(line, "Content-Type: %s;charset=IOS-8859-1\r\n", content_type[0]);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	int len = 0;
	struct stat st;
	if(stat(PAGE_201, &st) == 0)
	{
		len = st.st_size;
	}
	sprintf(line, "Content-Length: %d\r\n\r\n", len+source_len);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));
	
	printf("开始读文件\n");
	FILE *fp = fopen(PAGE_201, "r");
	int s = 0;
	int num = 0;
	char keys_front[1024] = "<input class=\"wow fadeInRight\" data-wow-delay=\"0.5s\" type=\"text\" value=\"";
	printf("keys_front's  length is %d\n", strlen(keys_front));
	char keys_latter[128] = "\"/>\r\n";
	printf("keys_latter's  length is %d\n", strlen(keys_latter));
	while(fgets(line, sizeof(line), fp) != NULL)
	{
		int l = strlen(line);
		num += l;
		if(strncasecmp(line, keys_front, strlen(keys_front)) == 0)
		{
//			printf("line's length is %d\n", l);
//			printf("%s", line);
			write(sock, keys_front, strlen(keys_front));
			write(sock, source, source_len);
			write(sock, keys_latter, strlen(keys_latter));
			num += source_len;
		}
		else
		{
			write(sock, line, strlen(line));
		}
		memset(line, 0x00, sizeof(line));
	}
	fclose(fp);

	printf("文件已关闭, 写了%d个字符\n", num);
}


void error_handle(int status_code, int sock)
{
	switch(status_code)
	{
	case 400:
		break;
	case 404:
		echo_www(sock, PAGE_404, 404);	
		break;
	case 500:
		echo_www(sock, PAGE_500, 200);
		break;
	default:
		break;
	}
}
int exe_cgi(int sock, char *path)
{
	return 200;
}


void echo_www_register(int sock)
{
	char line[4096];
	char source[50] = {0};
	char page[] = "wwwroot/log_about/register.html";
	sprintf(source, "alert(\"该用户名已存在，请重新输入\");");
	sprintf(line, "HTTP/1.0 200 OK\r\n");
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	sprintf(line, "Content-Type: %s;charset=IOS-8859-1\r\n", content_type[0]);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	int len = 0;
	int source_len = strlen(source);
	struct stat st;
	if(stat(page, &st) == 0)
	{
		len = st.st_size;
	}
	printf("source_len=%d\n", source_len);
	sprintf(line, "Content-Length: %d\r\n\r\n", len+source_len);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	printf("开始读文件, %s\n", page);
	FILE *fp = fopen(page, "r");
	int s = 0;
	int num = 0;
	char keys[] = "<script type=\"text/javascript\">";
	while(fgets(line, sizeof(line), fp) != NULL)
	{
		int l = strlen(line);
		if(strncasecmp(line, keys, strlen(keys)) == 0)
		{
			printf("匹配到\n");
			write(sock, line, strlen(line));
			write(sock, source, source_len);
			num += source_len;
		}
		else
		{
			write(sock, line, strlen(line));
		}
		num += l;
		memset(line, 0x00, sizeof(line));
	}
	fclose(fp);

	printf("文件已关闭, 写了%d个字符\n", num);
}

//urls:用户所上传的图片url
void echo_www_loged(int sock, char *username, char *passwd, char *urls)
{
	char show_user[150] = {0};
	char line[4096] = {0};
	char page[] = "wwwroot/user_index.html";
	int num_url = 0;         //图片的数量
	char str_tip[] = "<h2> 第一个url为您最近上传的图片</h2>\r\n";
	printf("echo_www_loged获得参数，sock=%d, username:%s, urls:%s\n", sock, username, urls);
	char source[] = "<input class=\"wow fadeInRight\" data-wow-delay=\"0.5s\" type=\"text\" value=\"\" name=\"我的图片\"/><br>\r\n";
	sprintf(show_user, "<li><a href=\"#\" name=\"user\">你好，亲爱的%s</a></li>\r\n", username);
	sprintf(line, "HTTP/1.0 200 OK\r\n");
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));

	sprintf(line, "Content-Type: %s;charset=IOS-8859-1\r\n", content_type[0]);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));
	int len = 0;
	int all_len = 0;
	struct stat st;
	if(stat(page, &st) == 0)
	{
		len = st.st_size;
	}
	all_len += len;
	all_len += strlen(username);
	all_len += strlen(username);
	if(passwd != NULL)
		all_len += strlen(passwd);
	if(urls != NULL)
	{
		num_url = 1;   //urls不为空，则至少有一个图片
		int i = 0;
		for(i = 0; i<strlen(urls); ++i)
		{
			if(urls[i] == '&')
				num_url += 1;
		}
		all_len = all_len + strlen(urls) - (num_url-1);
		all_len =  all_len + (num_url-1)*strlen(source); //文件中本身就有一个source，所以减1
		all_len += strlen(str_tip);
	}
	sprintf(line, "Content-Length: %d\r\n\r\n", all_len);
	write(sock, line, strlen(line));
	memset(line, 0x00, sizeof(line));
	printf("all_len=%d\n", all_len);
	printf("num_url=%d\n", num_url);
	printf("开始读文件, %s\n", page);
	FILE *fp = fopen(page, "r");
	int num = 0;          //已发送的字符串数量
	int sign1 = 0;        //标志着是否已发送用户名，0为还没发
	int sign2 = 0;        //标志着是否已发送图片url，0为还没发
	int sign3 = 0;        //标志着是否已经发送隐藏表单域中的用户名密码, 0为还没发
	int sign4 = 0;        //标志着是否已经发送“最新图片”的提示信息， 0为还没发
	while(fgets(line, sizeof(line), fp) != NULL)
	{
		if(sign1 == 0)
		{
			char str[] = "<li><a href=\"#\" name=\"user\">你好，亲爱的</a></li>";
			if(strncasecmp(line, str, strlen(str)) == 0)
			{
				write(sock, show_user, strlen(show_user));
				sign1 = 1;
				printf("前：[%s]\n", line);
				printf("后：[%s]\n", show_user);
				num += strlen(show_user);
				memset(line, 0x00, sizeof(line));
				continue;
			}
		}
		if(sign2 == 0 && urls != NULL)
		{
			if(strncasecmp(line, source, strlen(source)) == 0)
			{
			// 切分并组装图片url，然后发送
				int i = 0;
				char show_url[400] = {0};
				sprintf(show_url, "<input class=\"wow fadeInRight\" data-wow-delay=\"0.2s\" type=\"text\" value=\"http://%s:%d/%s\" name=\"我的图片\"/><br>\r\n", IP, PORT, strtok(urls, "&"));
				write(sock, show_url, strlen(show_url));
				printf("show_url:%s\n", show_url);
				num += strlen(show_url);
				for(i = 0; i<num_url-1; i++)
				{
					memset(show_url, 0x00, sizeof(show_url));	
					sprintf(show_url, "<input class=\"wow fadeInRight\" data-wow-delay=\"0.2s\" type=\"text\" value=\"http://%s:%d/%s\" name=\"我的图片\"/><br>\r\n", IP, PORT, strtok(NULL, "&"));
					write(sock, show_url, strlen(show_url));
					printf("show_url:%s\n", show_url);
					num += strlen(show_url);
				}
				sign2 = 1;
				memset(line, 0x00, sizeof(line));
				continue;
			}
		}
		if(sign3 == 0)
		{
			char str1[] = "<input type=\"hidden\" name=\"username\" value=\"\">\r\n";
			char str2[] = "<input type=\"hidden\" name=\"passwd\" value=\"\">\r\n";
			if(strncasecmp(line, str1, strlen(str1)) == 0)
			{
				char show_name[400] = {0};
				sprintf(show_name, "<input type=\"hidden\" name=\"username\" value=\"%s\">\r\n", username);
				write(sock, show_name, strlen(show_name));
				num += strlen(show_name);
				memset(line, 0x00, sizeof(line));
				continue;
			}
			if(strncasecmp(line, str2, strlen(str2)) == 0)
			{
				char show_passwd[400] = {0};
				sprintf(show_passwd, "<input type=\"hidden\" name=\"passwd\" value=\"%s\">\r\n", passwd);
				write(sock, show_passwd, strlen(show_passwd));
				num += strlen(show_passwd);
				memset(line, 0x00, sizeof(line));
				sign3 = 1;
				continue;
			}
		}
		if(sign4 == 0 && urls != NULL)
		{
			char str[] = "<h1> <label>您的图片</label></h1>\r\n";
			if(strncasecmp(line, str, strlen(str)) == 0)
			{
				write(sock, line, strlen(line));
				num += strlen(line);
				write(sock, str_tip, strlen(str_tip));
				num += strlen(str_tip);
				memset(line, 0x00, sizeof(line));
				sign4 = 1;
				continue;
			}
		}

		write(sock, line, strlen(line));
		num += strlen(line);
		memset(line, 0x00, sizeof(line));
	}
	fclose(fp);

	printf("文件已关闭, 已经发送了%d个字符\n", num);

}

void echo_www_loged_with_register(int sock, char *username)
{ 
	printf("echo_www_loged 获得参数，sock:%d, username:%s\n", sock, username);
	echo_www_loged(sock, username, NULL, NULL);
}

/*
 返回值由cgi的结果而来，500为没找到
 200为找到
 */
int get_urls_by_name(char *query_string, int query_len, char *username)
{
	int output[2];
	pipe(output);
	pid_t pid = fork();
	if(pid < 0)
		return 500;
	if(pid == 0)
	{
		close(output[0]);
		dup2(output[1], 1);
		execl("cgi/sql_connect/get_urls", "cgi/sql_connect/get_urls", username, NULL);
		perror("execl 失败");
		exit(500);
	}
	close(output[1]);
	wait(NULL);
	char tmp[1024] = {0};
	ssize_t s = read(output[0], tmp, sizeof(tmp));
	tmp[s] = 0;
	close(output[0]);
	int status = atoi(tmp);
	if(status == 200)
		sprintf(query_string ,"%s", tmp+3);
	return status;
}

//解析path中的用户名和cooike，通过用户名查最多查10个图片url
//然后拼接echo_www_loged需要的参数形式
void echo_www_loged_with_log(int sock, char *path)
{
	char passwd[33] = {0};
	char username[30] = {0};
	char urls[1024] = {0};     //传入get_url_by_name中获得用户上传图片的URL
	printf("echo_www_loged_with_log 获得参数，sock:%d, path:%s\n", sock, path);
	sprintf(username, "%s", strtok(path, "&"));
	sprintf(passwd, "%s", strtok(NULL, "&"));
	printf("username:%s   passwd:%s\n", username, passwd);
	int status = get_urls_by_name(urls, sizeof(urls), username);
	printf("get_urls_by_name的返回结果为：%d     %s", status, urls);
	if(status == 500)      //用户没有上传过图片
	{
		echo_www_loged(sock, username, passwd,  NULL);
	}
	else   //用户有图片
	{	
		int len = strlen(urls);	
		urls[len-1] = 0;
		printf("用户登录后上传图片\n");
		echo_www_loged(sock, username, passwd, urls+1);
	}
}

/*
  两种情况：
  该用户已存在
  注册成功
 */
void register_response(int sock, char *path)
{
	printf("用户注册响应参数， sock=%d  path=%s\n", sock, path);
	char str[] = "该用户已存在";
	if(strncmp(path, str, 6*3) == 0) //用户名重复，返回注册页面并附加提示信息
	{
		echo_www_register(sock);	
	}
	else    //注册成功，返回已登录页面
	{
		char username[30] = {0};
		strcpy(username, path+4*3);
		printf("register_response中， path:%s\n", path);
		echo_www_loged_with_register(sock, username);
	}
}

/*
   登录失败返回失败页面
   登录成功返回成功页面
 */
void login_response(int sock, char *path)
{
	printf("login_response获得参数，sock=%d  path:%s    status_code=%d\n", sock, path);
	if(strncmp(path, "登录失败", 4*3) == 0)
	{
		char path[] = "wwwroot/log_about/login_error.html";
		echo_www(sock, path, 200);	
	}
	else if(strncmp(path, "登录成功", 4*3) == 0)
	{
		echo_www_loged_with_log(sock, path+4*3+1);
	}
	else
	{
		echo_www(sock, PAGE_400, 200);
	}
}


void userSave_response(int sock, char *path)
{
	printf("userSave_response获得参数, sock=%d  path:%s\n", sock, path);
	echo_www_loged_with_log(sock, path);
}

void handler_response(int epfd, int sock)
{
	char *method = head_haxi.arry[sock].method;
	char *url = head_haxi.arry[sock].url;
	char *query_string = head_haxi.arry[sock].query_string;
	int id = head_haxi.arry[sock].imgid;
	int status_code = head_haxi.arry[sock].status_code;
	int clos = head_haxi.arry[sock].clos;
	int response_code = 200;
	char path[MAX_COLS/4] = {0};
	//show_time();	
	printf("获得参数：method: %s    url: %s    query_string: %s    id: %d    status_code: %d    sock:%d    close:%d\n", method, url, query_string, id, status_code, sock, clos);
	int cgi = 0;
	if(clos)
		goto end;
	if(status_code != 200)
		goto error;
	if(method == NULL || url == NULL)
	{
		status_code = 500;
		goto error;
	}
	else if(strcasecmp(method, "get") == 0 || strcasecmp(method, "HEAD") == 0) //get方法
	{
		if(strcmp(query_string, "\0") != 0)    //get方法带参数，即要执行服务器程序
		{
			cgi = 1;
			printf("get、cgi=1：method: %s    url: %s    query_string: %s    id: %d    status_code: %d    sock:%d    close:%d\n", method, url, query_string, id, status_code, sock, clos);
		}
		else  //发送普通文件，文件在images下和wwwroot下
		{	  //images 下的图片是用户上传的，url自带/images/
			  //所以首先尝试添加wwwroot,若找不到文件，则去掉wwwroot，直接找url下的（images），若还找不到就404
			sprintf(path, "wwwroot%s", url); 
			if(path[strlen(path)-1] == '/')
				strcat(path, HOME_PAGE);
			printf("reponse中，method:%s   url:%s   query_string:%s   path:%s\n", method, url, query_string, path);
			struct stat st;
			if(stat(path, &st) == 0)  //添加wwwroot后能打开，说明是wwwroot下的静态文件
			{	
				if(S_ISDIR(st.st_mode))
				{
					strcat(path, "/");
					strcat(path, HOME_PAGE);
					struct stat st2;
					if(stat(path, &st2) < 0)
					{
						status_code = 404;
						goto error;
					}
				}
				
			}
			else //添加wwwroot后不能打开，则去掉wwwroot/后再次尝试
			{
				int len = strlen(path);
				printf("去掉wwwroot/前，path:%s\n", path);
				int i = 0;
				for(i = 0; i<len-8; i++)
				{
					path[i] = path[i+8];
				}
				path[i] = 0;
				printf("去掉wwwroot/后，i=%d, path:%s\n", i, path);
				if(stat(path, &st) < 0)
				{
					status_code = 404;
					goto error;
				}
				else if(st.st_mode & S_IXUSR ||
						st.st_mode & S_IXGRP ||
						st.st_mode & S_IXOTH)
				{
					cgi = 1;
					printf("文件%s具有可执行属性\n", path);
				}

			}

		}
	}
	else if(strcasecmp(method, "post") == 0) //post方法
	{
		printf("reponse中，method:%s   url:%s   query_string:%s   path:%s\n", method, url, query_string, path);
		sprintf(path, "%s", query_string);
		if(strcmp(url, "/save") == 0)  
		{
			response_code = 201;
		}
		else if(strcmp(url, "/log_about/register") == 0)
		{
			response_code = 202;
		}
		else if(strcmp(url, "/log_about/login") == 0)
		{
			response_code = 203;
			
		}
		else if(strcmp(url, "/log_about/save") == 0)
		{
			response_code = 204;
		}
		else
		{
			status_code = 400;
			goto error;
		}
	}
	else //其它方法
	{
	}
		
//************以下进行200响应 **********

	if(cgi)
	{
		printf("do exe_cgi (%s), status_code=%d\n", path, status_code);
		status_code = exe_cgi(sock, path);
	}
	else if(status_code == 200)
	{
		printf("echo_www %d    %s   response_code:%d\n", sock, path, response_code);
		if(response_code == 200)
			echo_www(sock, path, 200);   //发送普通文件

		if(response_code == 201)
			echo_www_url(sock, path);     //上传成功界面
		if(response_code == 202)          //用户注册
		{
			register_response(sock, path);
		}
		if(response_code == 203)
		{
			login_response(sock, path);
		}
		if(response_code == 204)
		{
			userSave_response(sock, path);
		}

	}
//************以上进行200响应 **********
error:
	if(status_code != 200 && status_code != 201)
	{
		printf("error_handle, status_code:%d	sock:%d\n", status_code, sock);
		error_handle(status_code, sock);
	}
end:
	if(haxiDelete(&head_haxi, sock) < 0)
		printf("head_haxi删除%d失败\n", sock);
	else 
		printf("head_haxi删除%d成功\n", sock);
	close(sock);
	epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL);
	printf("****************************************************%d链接已关闭*******************************************************************************\n", sock);
}

void handler_events(int epfd, struct epoll_event events[], int size, int sock)
{
	int i = 0;
	for(i; i<size; i++)
	{
		if(events[i].data.fd == sock && (events[i].events & EPOLLIN) == 1)
		{
//			printf("监听事件就绪\n");
			handler_accept(epfd, events[i].data.fd);
		}
		else
		{
			if(events[i].events & EPOLLIN)
			{
				//show_time();
				printf("读事件就绪, ******************************************sock=%d******************************************\n", events[i].data.fd);
				handler_request(epfd, events[i].data.fd);
			}
			if(events[i].events & EPOLLOUT)
			{
				//show_time();
				printf("写事件就绪i, *****************************************sock=%d******************************************\n", events[i].data.fd);
				handler_response(epfd, events[i].data.fd);
			}
		}
	}
}


int main(int argc, char *argv[])
{
	if(argc != 1)
	{
		printf("Usage: %s\n", argv[0]);
		return 1;
	}

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		perror("socket");
		return 2;
	}
	struct sockaddr_in local;
	local.sin_family = AF_INET;   //类型
	local.sin_port = htons(PORT);  //端口号
	local.sin_addr.s_addr = INADDR_ANY;  //ip地址
//	local.sin_addr.s_addr = inet_addr(IP);  //ip地址
	if(set_noblock(sock) < 0)
	{
		perror("fcntl");
		return 5;
	}

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0)
	{
		perror("bind");
		return 3;
	}
	if(listen(sock, 25) < 0)
	{
		perror("listen");
		return 4;
	}
	haxiInit(&head_haxi, 10);
//	imgid = getId();
	printf("初始化获得imgid：%d, 睡2秒\n", imgid);
	sleep(2);
	int epfd = epoll_create(MAX_EVENTS);
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event);	
	
	struct epoll_event events[MAX_EVENTS-1];
	int timeout = 1000;
	while(1)
	{
		int size = epoll_wait(epfd, events, MAX_EVENTS-1, timeout);
		if(size < 0)
		{
			perror("epoll_wait");
		}
		else if(size == 0)
		{
			printf("timeout, listen_sock=%d\n", sock);
		}
		else
		{
			handler_events(epfd, events, size, sock);
			printf("触发\n");
		}

	}

	printf("return 0\n");
	return 0;
}


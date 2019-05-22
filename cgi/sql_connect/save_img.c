#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include "mysql.h"
#define IMGPATH "images/"
#define TABLE "image"

char img_type[][10] = {
	{".gif"},
	{".jpg"},
	{".png"}};

char key[27] = "qwertyuiopasdfghjklzxcvbnm";  //加密密匙

int get_line(int sock, char buf[], int cols, int *cl)
{
	int count = 0;
	char ch = '1';
	while(ch != '\n' && count < cols - 1)
	{   
		if(recv(sock, &ch, 1, 0) <= 0)
			continue;
		if(ch == '\r')
		{   
			recv(sock, &ch, 1, MSG_PEEK);
			if(ch == '\n')
			{
				recv(sock, &ch, 1, 0); 
				if(cl != NULL)
					*cl = 2;
			}
			else
				ch = '\n';          
		}   
		buf[count++] = ch; 
	}   
	buf[count] = '\0';
	return count;
}

int get_imgid(char *imgid, int len)
{
	MYSQL *sqlfd = mysql_init(NULL);
	if(mysql_real_connect(sqlfd, "127.0.0.1", "root", "123", "halou_bed", 3306, NULL, 0) == NULL)
	{
    	mysql_close(sqlfd);
		return -1;
	}
	char sql[1024] = {0};
    sprintf(sql, "select MAX(id) from image");
	if(mysql_query(sqlfd, sql) < 0)
	{
  		mysql_close(sqlfd);
		return -1;
	}
    MYSQL_RES *res = mysql_store_result(sqlfd);
    if(res == NULL)
    {
    	mysql_close(sqlfd);
		return -1;
    }
    MYSQL_ROW line;
    line = mysql_fetch_row(res);
    if(line[0] == NULL)
    {
    	mysql_close(sqlfd);
		return -1;
    }
    int maxid = atoi(line[0]);
    mysql_close(sqlfd);
	sprintf(imgid, "%d\0", maxid+1);
	return 0;
}

int  get_type_from_head(int sock, char *type, int type_len, char *boundary, int *cl)
{
	char line[10240];
	char *typebuf;
	int num = 0;
	memset(line, 0x00, 10240);
	get_line(sock, line, 10240, cl);

	memset(line, 0x00, 10240);
	get_line(sock, line, 10240, NULL);	
	num += strlen(line);

	memset(line, 0x00, 10240);
	get_line(sock, line, 10240, NULL);		
	num += strlen(line);

	strtok(line, "/");
	typebuf = strtok(NULL, "/");
	typebuf[strlen(typebuf)-1] = 0;
	if(strcmp(typebuf, "gif") == 0)
	{
		sprintf(type, ".gif");
	}
	else if(strcmp(typebuf, "jpeg") == 0)
	{
		sprintf(type,".jpg");
	}
	else if(strcmp(typebuf, "png") == 0)
	{
		sprintf(type, ".png");
	
	}
	memset(line, 0x00, 1024);
	get_line(sock, line, 10240, NULL);		
	num += strlen(line);

	return num;
}

int main(int argc, char *argv[])
{
	int status_code = 500;
	strtok(argv[1], "=&");
	int sock = atoi(strtok(NULL, "=&"));
	strtok(NULL, "=&");
	int length = atoi(strtok(NULL, "=&"));
	strtok(NULL, "=&");
//	char *imgid  = strtok(NULL, "=&");
//	strtok(NULL, "=&");
	char *boundary = strtok(NULL, "=&");
	char type[20] = {0};
	int cl = 1;  //传入get_len中取得换行符所占的字节数
	int num =  0;
	num = get_type_from_head(sock, type, sizeof(type), boundary, &cl);
	char path[1024] = IMGPATH;
    char imgname[40] = {0};
		int head_len = num+strlen(boundary)+1+2+4*(cl-1);
	int body_len = length - (num+2*strlen(boundary) + 4*(cl-1)+2*cl+1+2*3); 
	int tail_len = strlen(boundary) + 2*2 + 2*cl; 
	num += 2*strlen(boundary);	
	num += 4*(cl-1) + 2*3 + 1 + 2*cl;
	char c;
	int count = 0;

	char *buf = (char *)malloc(sizeof(char)*body_len+1);
	if(buf == NULL)
		goto end;
	while(count < body_len)
	{
		ssize_t ss = read(sock, &c, 1);
		if(ss < 0)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
		}
		else
		{
			buf[count] = c;
			count++;
		}
	}

    char imgid[21] = {0};
	int res = get_imgid(imgid, 20);
	if(res == -1)
	{
		goto end;
	}

    int i = 0;
    strcat(imgname, "baobao");
//	printf("imgid is %s\n", imgid);
//	printf("strlen(imgid) is %d\n", strlen(imgid));
    for(i = 0; i<strlen(imgid); i++)
    {
        imgid[i] += ('a' - '0');
    }
//	printf("imgid + 'a' is: %s\n", imgid);
    strcat(imgname, imgid);
    strcat(imgname, "xiangni");
//	printf("imgname1 is: %s\n", imgname);
    for(i = 0; i<strlen(imgname); i++)
    {
		imgname[i] = key[imgname[i] - 'a'];
    }
	strcat(imgname, type);
//	printf("path1 is: %s\n", path);
	strcat(path, imgname);
//	printf("imgname2 is: %s\n", imgname);
//	printf("path2 is: %s\n", path);
//	fflush(stdout);
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd < 0)
	{
		goto end;
	}
	write(fd, buf, body_len);	
	close(fd);

	count = 0;
	while(count < tail_len)
	{
		ssize_t s = read(sock, &c, 1);
		if(s > 0)
			count++;
	}

	MYSQL *sqlfd = mysql_init(NULL);
	if(mysql_real_connect(sqlfd, "127.0.0.1", "root", "123", "halou_bed", 3306, NULL, 0) == NULL)
	{
    	mysql_close(sqlfd);
		goto end;
	}
	char sql[1024] = {0};

	//这里的path是图片在服务器上的相对路径
	sprintf(sql, "insert into %s(path, user_id) values('%s', %d)", TABLE, path, 0);
	if(mysql_query(sqlfd, sql) < 0)
	{
		mysql_close(sqlfd);
		goto end;
	}
	mysql_close(sqlfd);
	status_code = 200;
end:	
	//返回给父进程该图片的路径和状态码即可
	printf("%s&%d", path, status_code);
	fflush(stdout);
	exit(0);
}

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

int get_userinfo(int sock, char *boundary, char *username, char *passwd, int *cl)
{
	int num = 0;
	char line[10240];
	num += get_line(sock, line, sizeof(line), cl);
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);   //Content-Disposition: form-data; name="username"\r\n\r\n
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);  //第四行为用户名
	printf("%s", line);
	sprintf(username, "%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl); //Content-Disposition: form-data; name="passwd"\r\n\r\n
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl); //第八行为密码
	printf("%s", line);
	sprintf(passwd, "%s", line);
	memset(line, 0x00, sizeof(line));


	return num;
}

int get_imgtype(int sock, char *boundary, char *type)
{
	char line[10240];
	int num = 0;

	num += get_line(sock, line, sizeof(line), NULL);
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), NULL);
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), NULL); //本行为Content-Type: image/png\r\n\r\n
	printf("%s", line);

	char *typebuf = NULL;
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
	memset(line, 0x00, sizeof(line));



	num += get_line(sock, line, sizeof(line), NULL);
	printf("%s", line);
	memset(line, 0x00, sizeof(line));

	return num;
}

int save_img(sock, img_len, path)
{
	char c;
	char *buf = (char *)malloc(sizeof(char)*img_len+1);
	int count = 0;
	if(buf == NULL)
		return -1;
	while(count < img_len)
	{   
		ssize_t ss = read(sock, &c, 1); 
		if(ss < 0)
		{   
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			else
				return -1;
		}   
		else
		{   
			buf[count] = c;
			count++;
		}   
	}   
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd < 0)
	{
		return -1;
	}
	write(fd, buf, body_len);
	close(fd);
	
	return 0;
}
int main(int argc, char *argv[])
{
	int status_code = 500;
	strtok(argv[1], "=&");
	int sock = atoi(strtok(NULL, "=&"));
	strtok(NULL, "=&");
	int length = atoi(strtok(NULL, "=&"));
	strtok(NULL, "=&");
	char *boundary = strtok(NULL, "=&");
	printf("存图片的cgi中，sock：%d, length:%d, boundary:%s\n", sock, length, boundary);
	char type[20] = {0};   //图片类型
	int cl = 1;  //传入get_len中取得换行符所占的字节数
	int num =  0;
	char username[100] = {0};
	char passwd[100] = {0};
	num += get_userinfo(sock, boundary, username, passwd, &cl);
	num += get_imgtype(sock, boundary, type);
	int front_len = num + 12;
	int tail_len = strlen(boundary) + 8;   //最后一行为 \r\n--boundary--\r\n
	int img_len = length - front_len - tail_len;
	printf("图片的大小为：%d\n", img_len);

	char imgname[40] = {0};
	char imgid[21] = {0};
	char path[1024] = IMGPATH;
	int res = get_imgid(imgid, 20);
	if(res == -1) 
	{   
		goto end;
	}   
	strcat(imgname, "baobao");
	int i = 0;
	for(i = 0; i<strlen(imgid); i++)
	{   
		imgid[i] += ('a' - '0');
	}   
	strcat(imgname, imgid);
	strcat(imgname, "xiangni");
	for(i = 0; i<strlen(imgname); i++)
	{   
		imgname[i] = key[imgname[i] - 'a'];
	}   
	strcat(imgname, type);
	strcat(path, imgname);
	printf("path:%s\n", path);



	int  = save_img(sock, img_len, path);

	//	MYSQL *sqlfd = mysql_init(NULL);
	//	if(mysql_real_connect(sqlfd, "127.0.0.1", "root", "123", "halou_bed", 3306, NULL, 0) == NULL)
	//	{
	//    	mysql_close(sqlfd);
	//		goto end;
	//	}
	//	char sql[1024] = {0};
	//
	//	//这里的path是图片在服务器上的相对路径
	//	sprintf(sql, "insert into %s(path, user_id) values('%s', %d)", TABLE, path, 0);
	//	if(mysql_query(sqlfd, sql) < 0)
	//	{
	//		mysql_close(sqlfd);
	//		goto end;
	//	}
	//	mysql_close(sqlfd);
	//	status_code = 200;
end:	
	//	//返回给父进程该图片的路径和状态码即可
	//	printf("%s&%d", path, status_code);
	fflush(stdout);
	exit(0);
}
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

int get_id(char *imgid, char *username, char *passwd, char *user_id, int len)
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
	sprintf(imgid, "%d\0", maxid+1);


	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "select user_id from user where name=\'%s\' and passwd=\'%s\';", username, passwd);
	if(mysql_query(sqlfd, sql) < 0)
	{
		mysql_close(sqlfd);
		return -1;
	}
	res = mysql_store_result(sqlfd);
	if(res == NULL)
	{
		mysql_close(sqlfd);
		return -1;
	}
	MYSQL_ROW line2;
	line2 = mysql_fetch_row(res);
	if(line2 == NULL)
	{
		mysql_close(sqlfd);
		return -1;
	}
	if(line2[0] == NULL)
	{
		mysql_close(sqlfd);
		return -1;
	}
	sprintf(user_id, "%s", line2[0]);

	mysql_close(sqlfd);
	return 0;
}

int get_userinfo(int sock, char *boundary, char *username, char *passwd, int *cl)
{
	int num = 0;
	char line[10240];
	num += get_line(sock, line, sizeof(line), cl);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);   //Content-Disposition: form-data; name="username"\r\n\r\n
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);  //第四行为用户名
	sprintf(username, "%s", line);
	username[strlen(username)-1] = 0;
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl); //Content-Disposition: form-data; name="passwd"\r\n\r\n
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), cl); //第八行为密码
	sprintf(passwd, "%s", line);
	passwd[strlen(passwd)-1] = 0;;
	memset(line, 0x00, sizeof(line));


	return num;
}

int get_imgtype(int sock, char *boundary, char *type)
{
	char line[10240];
	int num = 0;

	num += get_line(sock, line, sizeof(line), NULL);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), NULL);
	memset(line, 0x00, sizeof(line));

	num += get_line(sock, line, sizeof(line), NULL); //本行为Content-Type: image/png\r\n\r\n

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
	memset(line, 0x00, sizeof(line));

	return num;
}

int save_img(int sock, int img_len, char *path)
{
	char c;
	char *buf = (char *)malloc(sizeof(char)*img_len+1);
	int count = 0;
	if(buf == NULL)
		return 0;
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
		return 0;
	}
	write(fd, buf, img_len);
	close(fd);
	
	return 1;
}
int main(int argc, char *argv[])
{
	strtok(argv[1], "=&");
	int sock = atoi(strtok(NULL, "=&"));
	strtok(NULL, "=&");
	int length = atoi(strtok(NULL, "=&"));
	strtok(NULL, "=&");
	char *boundary = strtok(NULL, "=&");
	char type[20] = {0};   //图片类型
	int cl = 1;  //传入get_len中取得换行符所占的字节数
	int num =  0;
	char username[100] = {0};
	char passwd[100] = {0};
	int status = 200;
	num += get_userinfo(sock, boundary, username, passwd, &cl);
	num += get_imgtype(sock, boundary, type);
	int front_len = num + 12;
	int tail_len = strlen(boundary) + 8;   //最后一行为 \r\n--boundary--\r\n
	int img_len = length - front_len - tail_len;

	char path[1024] = IMGPATH;
	char imgname[21] = {0};
	char imgid[21] = {0};
	char userid[21] = {0};
	int res = get_id(imgid, username, passwd, userid, 20);
	if(res == -1) 
	{   
		status = 500;
	//	printf("1111111111111111111\n");
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

	if(save_img(sock, img_len, path) != 1)
	{
		status = 500;
	//	printf("2222222222222222222\n");
		goto end;
	}

	int count = 0;
	while(count < tail_len)
	{
		char c;
		ssize_t s = read(sock, &c, 1);
		if(s < 0)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			else 
				break;
		}
		count++;
	}


	MYSQL *sqlfd = mysql_init(NULL);
	if(mysql_real_connect(sqlfd, "127.0.0.1", "root", "123", "halou_bed", 3306, NULL, 0) == NULL)
	{
		mysql_close(sqlfd);
		status = 500;
	//	printf("3333333333333333333\n");
		goto end;
	}
	char sql[1024] = {0};
	
	//这里的path是图片在服务器上的相对路径
	sprintf(sql, "insert into %s(path, user_id) values('%s', '%s')", TABLE, path, userid);
	if(mysql_query(sqlfd, sql) < 0)
	{
		mysql_close(sqlfd);
		status = 500;
	//	printf("4444444444444444444\n");
		goto end;
	}
	mysql_close(sqlfd);
end:	
	printf("%d&%s&%s", status, username, passwd);
	fflush(stdout);
	exit(0);
}

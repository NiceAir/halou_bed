#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "mysql.h"


int main(int argc, char *argv[])
{
	char username[35] = {0};
	sprintf(username, "%s", argv[1]);
	MYSQL *sqlfd = mysql_init(NULL);
	if(mysql_real_connect(sqlfd, "127.0.0.1", "root", "123", "halou_bed", 3306, NULL, 0) == NULL)
	{   
		mysql_close(sqlfd);
		printf("500&链接数据库失败");
		return -1;
	}   
	char sql[1024] = {0};
	/*
	 select image.path, image.user_id
	 from image where image.user_id=(select user_id from user where name='q');
	 */
	sprintf(sql, "select image.path from image where image.user_id=(select user_id from user where name='%s');", username); 
	if(mysql_query(sqlfd, sql) < 0)
	{
		mysql_close(sqlfd);
		printf("500&执行sql语句失败");
		return -1;
	}
	MYSQL_RES *res = mysql_store_result(sqlfd);
	if(res == NULL)
	{
		mysql_close(sqlfd);
		printf("500&获取结果集失败");
		return -1;
	}
	MYSQL_ROW line;
	int status = 500;
	char out[1024] = {0};
	while(line = mysql_fetch_row(res))
	{
		status = 200;
		strcat(out, line[0]);
		strcat(out, "&");
	}
	mysql_close(sqlfd);
	printf("%d&%s", status, out);
	return 0;

}



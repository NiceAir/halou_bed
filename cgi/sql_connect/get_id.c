#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "mysql.h"

int main()
{
	MYSQL *sqlfd = mysql_init(NULL);	
	if(NULL == mysql_real_connect(sqlfd, "127.0.0.1", "root", "123", "halou_bed", 3306, NULL, 0))
	{
//		printf("error: %s\n", mysql_error(sqlfd));
		printf("-1");
		goto end;
	}		
	char query[1024] = {0};
	sprintf(query, "select MAX(id) from image");
	if(mysql_query(sqlfd, query) < 0)
	{
		printf("-2");
		goto end;
	}	

	MYSQL_RES *res = mysql_store_result(sqlfd);
	if(res == NULL)
	{
		printf("-3");
		goto end;
	}

	MYSQL_ROW line;
	line = mysql_fetch_row(res);
	if(line[0] == NULL)
		printf("12");
	else printf("%s", line[0]);

end:
	mysql_close(sqlfd);
	return 0;
}

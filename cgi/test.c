#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

int main()
{
	pid_t pid = fork();
	if(pid == 0)
	{
		execl("math_cgi", "math_cgi", "a=10&b=100", NULL);
		exit(1);
	}
	else
	{
		wait();
	}
}

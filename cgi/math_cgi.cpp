#include<iostream>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
using namespace std;

int main(int argc, char *argv[])
{
	//由于是通过exec传进来的参数，所以参数必然为"a=10&b=200"的形式
	printf("%s\n", argv[1]);
	strtok(argv[1], "=&");
	int a = atoi(strtok(NULL, "=&"));
	strtok(NULL, "=&");
	int b = atoi(strtok(NULL, "=&"));
	printf("%d + %d = %d\n", a, b, a+b);
	return 0;
}

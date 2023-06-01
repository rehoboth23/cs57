#include <stdio.h>

int func(int i);
void print(char i)
{
	printf("%d", i);
}

void printc(char c)
{
	printf("%c", c);
}

int read()
{
	int i;
	scanf("%d", &i);
	return i;
}

char readc()
{
	char c;
	scanf("%c", &c);
	return c;
}

int main()
{
	int i = func(3);
	printf("In main printing return value of test: %d\n", i);
	return 0;
}

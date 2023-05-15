#include <stdio.h>

// int add(int, char);

void print(char i)
{
	printf("-> %d\n", i);
}

void printc(char c)
{
	printf("-> %c\n", c);
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

// int main()
// {
// 	int i = add(3, 'c');
// 	printf("In main printing return value of test: %d\n", i);
// 	return 0;
// }

#include <stdio.h>

int test(int);

void print(int i)
{
	printf("-> %d\n", i);
}

int read()
{
	int i;
	scanf("%d", &i);
	return i;
}

int main()
{
	int i = test(4);
	printf("In main printing return value of test: %d\n", i);
	return 0;
}

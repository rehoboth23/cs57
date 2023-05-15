extern void print(int);
extern int read();
extern char readc();
extern void printc(char);

int main()
{
	// int a;
	// int b;
	// b  = 'c';
	// char b;
	// b = readc();
	// print(a);
	int b = read();
	if (b > 0)
	{
		while (b)
		{
			print(b);
			b = b - 1;
		}
	}
	else
		print(b);
	return 0;
}

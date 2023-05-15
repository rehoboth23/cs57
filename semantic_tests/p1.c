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
	print(b);
	print(b > 0);
	return 0;
}

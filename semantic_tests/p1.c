extern void print(int);
extern int read();
extern char readc();
extern void printc(char);

int main()
{
	int a;
	a = read();
	int b = read();
	print(((a + b) * 2));
	// if (read() - 2 > 3 * a - b)
	// {
	// 	print(a - 2);
	// 	while (b)
	// 	{
	// 		print(b + a + 1);
	// 		b = b - 1;
	// 	}
	// }
	return 0;
}

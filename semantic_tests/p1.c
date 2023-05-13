extern void print(int);
extern int read();

int test(int m)
{
	int a;
	int b;
	a = 20;
	if (m > a)
	{
		b = a;
		a = m - 6;
		return (a);
	}
	else
	{
		a = m + 6;
		return(19);
	}

	return (a);
}

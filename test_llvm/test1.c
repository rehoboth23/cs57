int func(int i)
{
	int a;
	int b;
	int c;
	int d;
	a = 10;
	b = 20;
	b = 20;
	b = 20;
	b = 20;
	b = 5;
	c = a + b;
	d = a + b;
	if (a < 10)
	{
		a = i;
		d = a + b;
		b = 20;
		return (b + a);
	}
	return c;
}

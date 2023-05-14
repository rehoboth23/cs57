extern void print(int);
extern int read();

int add(int m, int n)
{
	int a;
	a = m;
	int b;
	b = 10;
	if (m > 0) {
		a = fork();
		while (b > 0) {
			if (b > 1) {
				sleep(1);
				int i;
				i = 0;
				while(i < 5) {
					sleep(1);
					print(i);
					i = i + 1;
				}
				print(b);
				b = b - 1;
			}
			sleep(1);
			a = a - 1;
			print(a);
		}
		return -5;
	}
	wait();
	return m + n;
}

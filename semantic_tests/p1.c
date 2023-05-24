extern void print(int);
extern int read();
extern char readc();
extern void printc(char);

int main()
{
	int a;
	a = read();
	if (a > 0) {
		return(1);
	} else if (a < 0) {
		return(-4 - -2);
	}
	return(0);
}

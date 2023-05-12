extern int read();
extern void print(int);

int test(int m){
	int a;
	int b;
	b = read();
	print(b);
	if (m < 5){
		a = m;
	}
	else {
		a = 5;
	}
	return(a);
}

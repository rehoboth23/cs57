extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;

	a = 0;
	b = 0;

  if (a < i){
		read();
		while (b < i){
			b = -20 + a;
		}
		a = 10 + b;
	}
	else {
		if (b < i) 
			b = a;
	}
	return(1);
}
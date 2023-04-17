extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	
  if (a < i){
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
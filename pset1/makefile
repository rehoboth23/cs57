source = pset1

all: test

$(source).out: $(source).l $(source).y
	yacc -d -v -t $(source).y
	lex $(source).l
	gcc -o $(source).out lex.yy.c y.tab.c

test:
	make clean
	make $(source).out
	./$(source).out ../miniC/p1.c &> out1.c
	./$(source).out ../miniC/p2.c &> out2.c
	./$(source).out ../miniC/p3.c &> out3.c
	./$(source).out ../miniC/p4.c &> out4.c
	./$(source).out ../miniC/p5.c &> out5.c
	./$(source).out ../miniC/p_bad.c &> out_bad.c

clean:
	rm -rf lex.yy.c y.tab.c y.tab.h $(source).out y.output out*.c TRACE*

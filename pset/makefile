source = pset

all: test

$(source).out: $(source).l $(source).y
	yacc -d -v -t $(source).y
	lex $(source).l
	g++ -o $(source).out lex.yy.c y.tab.c ast.c

test:
	make clean
	make $(source).out
	./$(source).out ../miniC/p1.c 1> out1.c 2> TRACE1
	./$(source).out ../miniC/p2.c 1> out2.c 2> TRACE2
	./$(source).out ../miniC/p3.c 1> out3.c 2> TRACE3
	./$(source).out ../miniC/p4.c 1> out4.c 2> TRACE4
	./$(source).out ../miniC/p5.c 1> out5.c 2> TRACE5
	./$(source).out ../miniC/p_bad.c 1> out_bad.c 2> TRACE_BAD

clean:
	rm -rf lex.yy.c y.tab.c y.tab.h $(source).out y.output out*.c TRACE*

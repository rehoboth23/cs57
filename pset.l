%{
#include <stdio.h>
#include <math.h>
#include "ast.h"
#include "y.tab.h"
%}

%option yylineno
letter					[a-zA-Z]
alphanum 				[a-zA-Z0-9]
alphanum_us 		[a-zA-Z0-9_]
variable 				{letter}{alphanum_us}*
number					[0-9]+
empty 					^[ \t]*\n$
%%
"extern" {
	printf("%s", yytext);
	return EXTERN;
}

"return" {
	printf("%s", yytext);
	return RETURN;
}

"int" {
	printf("%s", yytext);
	char *p=(char *)calloc(strlen(yytext)+1,sizeof(char));
	strncpy(p, yytext, strlen(yytext));
	yylval.vname = p;
	return TYPE;
}
"void" {
	printf("%s", yytext);
	char *p=(char *)calloc(strlen(yytext)+1,sizeof(char));
	strncpy(p, yytext, strlen(yytext));
	yylval.vname = p;
	return TYPE;
}

"if" {
	printf("%s", yytext);
	return IF;
}
"else" {
	printf("%s", yytext);
	return ELSE;
}
"while" {
	printf("%s", yytext);
	return WHILE;
}

"*" {
	printf("%s", yytext);
	return MUL;
}
"/" {
	printf("%s", yytext);
	return DIV;
}
"-" {
	printf("%s", yytext);
	return SUB;
}
"+" {
	printf("%s", yytext);
	return ADD;
}

"<" {
	printf("%s", yytext);
	return LT;
}
">" {
	printf("%s", yytext);
	return GT;
}
"<=" {
	printf("%s", yytext);
	return LTE;
}
">=" {
	printf("%s", yytext);
	return GTE;
}
"==" {
	printf("%s", yytext);
	return EQ;
}
"!=" {
	printf("%s", yytext);
	return NEQ;
}

"(" {
	printf("%s", yytext);
	return yytext[0];
}
")" {
	printf("%s", yytext);
	return yytext[0];
}
";" {
	printf("%s", yytext);
	return yytext[0];
}
"{" {
	printf("%s", yytext);
	return yytext[0];
}
"}" {
	printf("%s", yytext);
	return yytext[0];
}
"=" {
	printf("%s", yytext);
	return yytext[0];
}

{variable} {
	printf("%s", yytext);
	char *p=(char *)calloc(strlen(yytext)+1,sizeof(char));
	strncpy(p, yytext, strlen(yytext));
	yylval.vname = p;
	return VAR;
}
{number} {
	printf("%s", yytext);
	yylval.ival = atoi(yytext);
	return NUM;
}
%%

int yywrap(){
	return 1;
}

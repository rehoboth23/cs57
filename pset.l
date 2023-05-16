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
"//".*\n|"/*"(.*\n*.*)*"*/" {
}

"extern" {
	// printf("%s", yytext);
	return EXTERN;
}

"return" {
	// printf("%s", yytext);
	return RETURN;
}

"char"|"int"|"void" {
	// printf("%s", yytext);
	char *p=(char *)calloc(strlen(yytext)+1,sizeof(char));
	strncpy(p, yytext, strlen(yytext));
	yylval.vname = p;
	return TYPE;
}

"if" {
	// printf("%s", yytext);
	return IF;
}
"else" {
	// printf("%s", yytext);
	return ELSE;
}
"while" {
	// printf("%s", yytext);
	return WHILE;
}
"," {
	return COMMA;
}
"*"|"/"|"+"|"-" {
	// printf("%s", yytext);
	char *p=(char *)calloc(strlen(yytext)+1,sizeof(char));
	strncpy(p, yytext, strlen(yytext));
	yylval.vname = p;
	return OP;
}

"!="|"=="|">="|"<="|">"|"<" {
	// printf("%s", yytext);
	char *p=(char *)calloc(strlen(yytext)+1,sizeof(char));
	strncpy(p, yytext, strlen(yytext));
	yylval.vname = p;
	return OP;
}

"(" {
	// printf("%s", yytext);
	return yytext[0];
}
")" {
	// printf("%s", yytext);
	return yytext[0];
}
";" {
	// printf("%s", yytext);
	return yytext[0];
}
"{" {
	// printf("%s", yytext);
	return yytext[0];
}
"}" {
	// printf("%s", yytext);
	return yytext[0];
}
"=" {
	// printf("%s", yytext);
	return yytext[0];
}

{variable} {
	// printf("%s", yytext);
	char *p=(char *)calloc(strlen(yytext)+1,sizeof(char));
	strncpy(p, yytext, strlen(yytext));
	yylval.vname = p;
	return VAR;
}
{number} {
	// printf("%s", yytext);
	yylval.ival = atoi(yytext);
	return NUM;
}
"'"({letter}|{number})"'" {
	yylval.ival = yytext[1];
	return CHAR;
}
"*" {
	return PTR;
}
%%

int yywrap(){
	return 1;
}

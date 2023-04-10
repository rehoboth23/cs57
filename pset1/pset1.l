%{
#include <stdio.h>
#include <math.h>
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
"extern"					{printf("%s", yytext);}

"int"							{printf("%s", yytext); 
				 return TYPE;}
"void"						{printf("%s", yytext); 
				 return TYPE;}

"if"							{printf("%s", yytext); 
				 return IF;}
"else"						{printf("%s", yytext); 
				 return ELSE;}
"while"						{printf("%s", yytext); 
				 return WHILE;}

"*" 							{printf("%s", yytext);
				return yytext[0];}
"/" 							{printf("%s", yytext);
				return yytext[0];}
"-" 							{printf("%s", yytext);
				return yytext[0];}
"+" 				{printf("%s", yytext);
				return yytext[0];}

"<" 							{printf("%s", yytext);
				return LT;}
">" 							{printf("%s", yytext);
				return GT;}
"<=" 							{printf("%s", yytext);
				return LTE;}
">=" 							{printf("%s", yytext);
				return GTE;}
"==" 							{printf("%s", yytext);
				return EQ;}

"("								{printf("%s", yytext); 
			 return yytext[0];}
")"								{printf("%s", yytext); 
       return yytext[0];}
";"   						{printf("%s", yytext); 
       return yytext[0];}
"{"   						{printf("%s", yytext); 
       return yytext[0];}
"}"   						{printf("%s", yytext); 
       return yytext[0];}
"="   						{printf("%s", yytext); 
       return yytext[0];}


{variable} 				{printf("%s", yytext);
					return VAR;}
{number}  				{printf("%s", yytext);
					return NUM;}
%%

int yywrap(){
	return 1;
}

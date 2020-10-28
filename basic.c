#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"


#ifdef _WIN32
#include <string.h>

static char buffer[2048]

char * readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char * cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

enum { LVAL_NUM, LVAL_DOUBLE, LVAL_ERR };

typedef struct {
	int type;
	union {
		long num;
		double dnum;
	} data;
	int err;
} lval;

lval lval_double(double x) {
	lval v;
	v.type = LVAL_DOUBLE;
	v.data.dnum = x;
	return v;
}

lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.data.num = x;
	return v;
}

lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

void lval_print(lval v) {
	switch(v.type) {
	case LVAL_NUM: printf("%li", v.data.num); break;
	case LVAL_DOUBLE: printf("%lf", v.data.dnum); break;
	case LVAL_ERR:
		switch(v.err){
			case LERR_DIV_ZERO: printf("Error: Division by zero not gud"); break;
			case LERR_BAD_OP: printf("Error: Not gud operator"); break;
			case LERR_BAD_NUM: printf("Error: Not gud number"); break;
		}
	}
}
lval double_ops(lval x, char* op, lval y) {
	double x0 = x.type == LVAL_DOUBLE ? x.data.dnum : (double) x.data.num;
	double y0 = y.type == LVAL_DOUBLE ? y.data.dnum : (double) y.data.num;
	
	if(strcmp(op, "+") == 0) { return lval_double(x0 + y0); }
	if(strcmp(op, "-") == 0) { return lval_double(x0 - y0); }	
	if(strcmp(op, "*") == 0) { return lval_double(x0 * y0); }
	if(strcmp(op, "/") == 0) { return y0 == 0 ? lval_err(LERR_DIV_ZERO) : lval_double(x0 / y0); }
	if(strcmp(op, "%") == 0) { return lval_double(fmod(x0, y0)); }
	if(strcmp(op, "^") == 0) { return lval_double(pow(x0,y0)); }
	if(strcmp(op, "min") == 0) { return (x0 <= y0) ? lval_double(x0) : lval_double(y0);}
	if(strcmp(op, "max") == 0) { return (x0 >= y0) ? lval_double(x0) : lval_double(y0);}
	
	return lval_err(LERR_BAD_OP);
}

lval int_ops(lval x, char* op, lval y) {
	if(strcmp(op, "+") == 0) { return lval_num(x.data.num + y.data.num); }
	if(strcmp(op, "-") == 0) { return lval_num(x.data.num - y.data.num); }	
	if(strcmp(op, "*") == 0) { return lval_num(x.data.num * y.data.num); }
	if(strcmp(op, "/") == 0) { return y.data.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.data.num / y.data.num); }
	if(strcmp(op, "%") == 0) { return lval_num(x.data.num % y.data.num); }
	if(strcmp(op, "^") == 0) { return lval_num(pow(x.data.num,y.data.num)); }
	if(strcmp(op, "min") == 0) { return (x.data.num <= y.data.num) ? lval_num(x.data.num) : lval_num(y.data.num);}
	if(strcmp(op, "max") == 0) { return (x.data.num >= y.data.num) ? lval_num(x.data.num) : lval_num(y.data.num);}
	
	return lval_err(LERR_BAD_OP);
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) { 
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	
	if(x.type == LVAL_NUM && y.type == LVAL_NUM) { return int_ops(x, op, y); }
	
	if(x.type == LVAL_DOUBLE || y.type == LVAL_DOUBLE) { return double_ops(x, op, y); }
	
	
	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){
	
	if (strstr(t->tag, "number")) {
	
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	printf("long: %ld \n", x);
	return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}
	
	if (strstr(t->tag, "decimal")) {
	
	errno = 0;
	double x = strtod(t->contents, NULL);
	printf("double: %f \n", x);
	return errno != ERANGE ? lval_double(x) : lval_err(LERR_BAD_NUM);
	}
	
	char* op = t->children[1]->contents;

	lval x = eval(t->children[2]);

 	int i = 3;
 	
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	
	if(strcmp(op, "-") == 0 && strstr(t->tag, "number")){ return lval_num(x.data.num*-1); }
	if(strcmp(op, "-") == 0 && strstr(t->tag, "decimal")){ return lval_double(x.data.num*-1); }
	return x;
}

int main(int argc, char** argv) {
	
	mpc_parser_t* Number   = mpc_new("number");
	mpc_parser_t* Decimal  = mpc_new("decimal");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr     = mpc_new("expr");
	mpc_parser_t* Lispy    = mpc_new("lispy");

	/* Define them with the following Language */
	mpca_lang(MPCA_LANG_DEFAULT,
	  " decimal  : /(\\d+)?\\.?\\d+/;                                                  \
	    number   : /-?[0-9]+/ ;                             \
	    operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\";            \
	    expr     : <decimal> | <number> | '(' <operator> <expr>+ ')' ;  \
	    lispy    : /^/ <operator> <expr>+ /$/ ;             \
	  ",
	Number, Decimal, Operator, Expr, Lispy);
	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl + C to Exit\n");


	while(1) {
		
		char* input = readline("lispy> ");

		add_history(input);

		mpc_result_t r;

		if (mpc_parse("<stdin>", input, Lispy, &r)) {

			lval result = eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);
						

			
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	mpc_cleanup(4, Number, Decimal, Operator, Expr, Lispy);
	return 0;
}

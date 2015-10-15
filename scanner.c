#include <stdio.h>
#include <stdlib.h> // just using for the exit function



// TODO 
//		ish ?? -- scientific notation reals ---- should be able to abuse numdigs (just decimals, then add intval with the 10^numdigs, then adjust numdigs)


typedef enum { false, true } bool;


const int nreswrd = 40;
const int inbuffsize = 256;
const int idbuffsize = 16;
// const int errmax = 256;
// const int intmax = 32767;

typedef enum {
	BOOLEAN_SYM, CHAR_SYM, FALSE_SYM, TRUE_SYM, NEW_SYM,
	REAL_SYM, INTEGER_SYM, HEXINT_SYM, ARRAY_SYM, IMPORT_SYM, THEN_SYM, BEGIN_SYM, IN_SYM,
	TO_SYM, BY_SYM, IS_SYM, CASE_SYM, MOD_SYM, TYPE_SYM, CONST_SYM,
	MODULE_SYM, UNTIL_SYM, DIV_SYM, NIL_SYM, VAR_SYM, DO_SYM, OF_SYM,
	WHILE_SYM, ELSE_SYM, OR_SYM, ELSIF_SYM, POINTER_SYM, END_SYM, 
	PROCEDURE_SYM, RECORD_SYM, FOR_SYM, REPEAT_SYM, IF_SYM, RETURN_SYM,
	EXIT_SYM, LOOP_SYM, WITH_SYM,

	lparen, rparen, plus, minus, mul, slash, rbrac, lbrac, equal, colon,
	lt, lte, gt, gte, semic, null, assign, carat, neq, comma, per, ident, 
	number, string, eof_sym, invalid_sym, op_sym, set_sym, tilde, 
	lcurb, rcurb, resword, doubledot  
} Token;

const char *reswrd[ 41][50]; // number of reswords
const char *symname[ 127][50];
const char *errmsg[ 256][32]; // 256 is errmax 
Token reswrdsym[ 41];
Token spsym[ 127]; 


FILE* fileIn;
char ch; // current char
Token sym; // current symbol
char inbuff[ 256]; // inbuffsize
char idbuff[ 16]; // idbuffsize
char strbuff[ 80]; // max size for string

int intval; // value of current int being read
int decimals;
int numdigs;

char hexBuff[ 256]; // let's make the hex numbers have a limit of 256 digs

int inptr;
int symptr;
int linelen = 0;
int linenum = 0;
int idlen = 16; // idbuffsize
int strlength = 0;

void InitErrMsgs() {
	int i;
	for( i = 0; i < 256; ++ i) 
		errmsg[ i][ 0] = "\0";

	errmsg[ 10][ 0] = "Error in general number format";
	errmsg[ 16][ 0] = "Error in hex number formatting";
	errmsg[ 30][ 0] = "Number too large";
	errmsg[ 39][ 0] = "39";
	errmsg[ 45][ 0] = "Unfinished comment... EOF reached";
	errmsg[ 50][ 0] = "String delimiter missing";
}

void InitSpSyms() { // one-char symbols (ie. not <=, >=, etc.)
	int i;
	for ( i = 0; i < 127; ++ i)
		spsym[ i] = null;

	spsym[ '('] = lparen;
	spsym[ ')'] = rparen;
	spsym[ '+'] = plus;
	spsym[ '-'] = minus;
	spsym[ '*'] = mul;
	spsym[ '/'] = slash;
	spsym[ ']'] = rbrac;
	spsym[ '['] = lbrac;
	spsym[ '='] = equal;
	spsym[ ':'] = colon;
	spsym[ '<'] = lt;
	spsym[ '>'] = gt;
	spsym[ ';'] = semic;
	spsym[ '^'] = carat;
	spsym[ '#'] = neq;
	spsym[ ','] = comma;
	spsym[ '.'] = per;
	spsym[ '~'] = tilde;
	spsym[ '{'] = lcurb;
	spsym[ '}'] = rcurb;
	spsym[ '|'] = OR_SYM;


}

void InitSymNames() {
	// initialize the table of spellings of symbols, required since
	// the symbols, being an enumerated type, cannot be written as such

	int i;
	for ( i = 0; i < 127; ++ i)
		symname[ i][ 0] = "\0"; // start by setting all entries to null string

	symname[   BOOLEAN_SYM][ 0] = "BOOLEAN_SYM";
	symname[      CHAR_SYM][ 0] = "CHAR_SYM";
	symname[     FALSE_SYM][ 0] = "FALSE_SYM";
	symname[      TRUE_SYM][ 0] = "TRUE_SYM";
	symname[       NEW_SYM][ 0] = "NEW_SYM";
	symname[      REAL_SYM][ 0] = "REAL_SYM";
	symname[   INTEGER_SYM][ 0] = "INTEGER_SYM";
	symname[    HEXINT_SYM][ 0] = "HEXINT_SYM";
	symname[     ARRAY_SYM][ 0] = "ARRAY_SYM";
	symname[    IMPORT_SYM][ 0] = "IMPORT_SYM";
	symname[      THEN_SYM][ 0] = "THEN_SYM";
	symname[     BEGIN_SYM][ 0] = "BEGIN_SYM";
	symname[        IN_SYM][ 0] = "IN_SYM";
	symname[        TO_SYM][ 0] = "TO_SYM";
	symname[        BY_SYM][ 0] = "BY_SYM";
	symname[        IS_SYM][ 0] = "IS_SYM";
	symname[      CASE_SYM][ 0] = "CASE_SYM";
	symname[       MOD_SYM][ 0] = "MOD_SYM";
	symname[      TYPE_SYM][ 0] = "TYPE_SYM";
	symname[     CONST_SYM][ 0] = "CONST_SYM";
	symname[    MODULE_SYM][ 0] = "MODULE_SYM";
	symname[     UNTIL_SYM][ 0] = "UNTIL_SYM";
	symname[       DIV_SYM][ 0] = "DIV_SYM";
	symname[       NIL_SYM][ 0] = "NIL_SYM";
	symname[       VAR_SYM][ 0] = "VAR_SYM";
	symname[        DO_SYM][ 0] = "DO_SYM";
	symname[        OF_SYM][ 0] = "OF_SYM";
	symname[     WHILE_SYM][ 0] = "WHILE_SYM";
	symname[      ELSE_SYM][ 0] = "ELSE_SYM";
	symname[        OR_SYM][ 0] = "OR_SYM";
	symname[     ELSIF_SYM][ 0] = "ELSIF_SYM";
	symname[   POINTER_SYM][ 0] = "POINTER_SYM";
	symname[       END_SYM][ 0] = "END_SYM";
	symname[ PROCEDURE_SYM][ 0] = "PROCEDURE_SYM";
	symname[    RECORD_SYM][ 0] = "RECORD_SYM";
	symname[       FOR_SYM][ 0] = "FOR_SYM";
	symname[    REPEAT_SYM][ 0] = "REPEAT_SYM";
	symname[        IF_SYM][ 0] = "IF_SYM";
	symname[    RETURN_SYM][ 0] = "RETURN_SYM";
	symname[      EXIT_SYM][ 0] = "EXIT_SYM";
	symname[      LOOP_SYM][ 0] = "LOOP_SYM";
	symname[      WITH_SYM][ 0] = "WITH_SYM";

	symname[        lparen][ 0] = "lparen";
	symname[        rparen][ 0] = "rparen";
	symname[          plus][ 0] = "plus";
	symname[         minus][ 0] = "minus";
	symname[           mul][ 0] = "mul";
	symname[         slash][ 0] = "slash";
	symname[         rbrac][ 0] = "rbrac";
	symname[         lbrac][ 0] = "lbrac";
	symname[         equal][ 0] = "equal";
	symname[         colon][ 0] = "colon";
	symname[            lt][ 0] = "lt";
	symname[           lte][ 0] = "lte";
	symname[            gt][ 0] = "gt";
	symname[           gte][ 0] = "gte";
	symname[         semic][ 0] = "semic";
	symname[          null][ 0] = "null";
	symname[        assign][ 0] = "assign";
	symname[         carat][ 0] = "carat";
	symname[           neq][ 0] = "neq";
	symname[         comma][ 0] = "comma";
	symname[           per][ 0] = "per";
	symname[         ident][ 0] = "ident";
	symname[        number][ 0] = "number";
	symname[        string][ 0] = "string";
	symname[       eof_sym][ 0] = "eof_sym";
	symname[   invalid_sym][ 0] = "invalid_sym";
	symname[        op_sym][ 0] = "op_sym";
	symname[       set_sym][ 0] = "set_sym";
	symname[         tilde][ 0] = "tilde";
	symname[         lcurb][ 0] = "lcurb";
	symname[         rcurb][ 0] = "rcurb";
	symname[       resword][ 0] = "resword";
	symname[     doubledot][ 0] = "doubledot";

}

void InitResWrds() {
	reswrd[ 0][ 0] = "BOOLEAN";
	reswrd[ 1][ 0] = "CHAR";
	reswrd[ 2][ 0] = "FALSE";
	reswrd[ 3][ 0] = "TRUE";
	reswrd[ 4][ 0] = "NEW";
	reswrd[ 5][ 0] = "REAL";
	reswrd[ 6][ 0] = "ARRAY";
	reswrd[ 7][ 0] = "IMPORT";
	reswrd[ 8][ 0] = "THEN";
	reswrd[ 9][ 0] = "BEGIN";
	reswrd[ 10][ 0] = "IN";
	reswrd[ 11][ 0] = "TO";
	reswrd[ 12][ 0] = "BY";
	reswrd[ 13][ 0] = "IS";
	reswrd[ 14][ 0] = "CASE";
	reswrd[ 15][ 0] = "MOD";
	reswrd[ 16][ 0] = "TYPE";
	reswrd[ 17][ 0] = "CONST";
	reswrd[ 18][ 0] = "MODULE";
	reswrd[ 19][ 0] = "UNTIL";
	reswrd[ 20][ 0] = "DIV";
	reswrd[ 21][ 0] = "NIL";
	reswrd[ 22][ 0] = "VAR";
	reswrd[ 23][ 0] = "DO";
	reswrd[ 24][ 0] = "OF";
	reswrd[ 25][ 0] = "WHILE";
	reswrd[ 26][ 0] = "ELSE";
	reswrd[ 27][ 0] = "OR";
	reswrd[ 28][ 0] = "ELSIF";
	reswrd[ 29][ 0] = "POINTER";
	reswrd[ 30][ 0] = "END";
	reswrd[ 31][ 0] = "PROCEDURE";
	reswrd[ 32][ 0] = "RECORD";
	reswrd[ 33][ 0] = "FOR";
	reswrd[ 34][ 0] = "REPEAT";
	reswrd[ 35][ 0] = "IF";
	reswrd[ 36][ 0] = "RETURN";
	reswrd[ 37][ 0] = "EXIT";
	reswrd[ 38][ 0] = "LOOP";
	reswrd[ 39][ 0] = "WITH";
	reswrd[ 40][ 0] = "INTEGER";
}

void InitResWrdSyms() {
	reswrdsym[ 0] = BOOLEAN_SYM;
	reswrdsym[ 1] = CHAR_SYM;
	reswrdsym[ 2] = FALSE_SYM;
	reswrdsym[ 3] = TRUE_SYM;
	reswrdsym[ 4] = NEW_SYM;
	reswrdsym[ 5] = REAL_SYM;
	reswrdsym[ 6] = ARRAY_SYM;
	reswrdsym[ 7] = IMPORT_SYM;
	reswrdsym[ 8] = THEN_SYM;
	reswrdsym[ 9] = BEGIN_SYM;
	reswrdsym[ 10] = IN_SYM;
	reswrdsym[ 11] = TO_SYM;
	reswrdsym[ 12] = BY_SYM;
	reswrdsym[ 13] = IS_SYM;
	reswrdsym[ 14] = CASE_SYM;
	reswrdsym[ 15] = MOD_SYM;
	reswrdsym[ 16] = TYPE_SYM;
	reswrdsym[ 17] = CONST_SYM;
	reswrdsym[ 18] = MODULE_SYM;
	reswrdsym[ 19] = UNTIL_SYM;
	reswrdsym[ 20] = DIV_SYM;
	reswrdsym[ 21] = NIL_SYM;
	reswrdsym[ 22] = VAR_SYM;
	reswrdsym[ 23] = DO_SYM;
	reswrdsym[ 24] = OF_SYM;
	reswrdsym[ 25] = WHILE_SYM;
	reswrdsym[ 26] = ELSE_SYM;
	reswrdsym[ 27] = OR_SYM;
	reswrdsym[ 28] = ELSIF_SYM;
	reswrdsym[ 29] = POINTER_SYM;
	reswrdsym[ 30] = END_SYM;
	reswrdsym[ 31] = PROCEDURE_SYM;
	reswrdsym[ 32] = RECORD_SYM;
	reswrdsym[ 33] = FOR_SYM;
	reswrdsym[ 34] = REPEAT_SYM;
	reswrdsym[ 35] = IF_SYM;
	reswrdsym[ 36] = RETURN_SYM;
	reswrdsym[ 37] = EXIT_SYM;
	reswrdsym[ 38] = LOOP_SYM;
	reswrdsym[ 39] = WITH_SYM;
	reswrdsym[ 40] = INTEGER_SYM;
}

void InitCompile() {
	InitResWrds();
	InitResWrdSyms();
	InitSpSyms();
	InitSymNames();
	InitErrMsgs();
}

void error( int eNum) {
	fputs( "------------------------------------\n", stdout);
	fputs( errmsg[ eNum][ 0], stdout);
	fputs( "\n------------------------------------\n", stdout);
	// sym = invalid_sym;
}


bool inSeparators( char c) {
	if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
		return true;
	return false;
}

bool inLowecase( char c) {
	return c >= 'a' && c <= 'z';
}

bool inUppercase( char c) {
	return c >= 'A' && c <= 'Z';
}

bool inLetters( char c) {
	return inLowecase(c) || inUppercase(c);
}

bool inDigits( char c) {
	return c >= '0' && c <= '9';
}

bool inHexDigits( char c) {
	return (c >= 'A' && c <= 'F') || inDigits( c);
}

void getLine() {
	// read the next line from the src file into inbuff

	++ linenum;

	inptr = 0;

	char temp = getc( fileIn);
	inbuff[ inptr] = temp;

	while (temp != '\n' && temp != EOF && temp != '\0') {
		temp = getc( fileIn);
		++ inptr;
		inbuff[ inptr] = temp;
	}

	++ inptr;
	inbuff[ inptr] = '\n';

	linelen = inptr;

	if (inptr == 1) // empty line
		linelen = 0;

	inptr = 0;

	ch = temp;
}

void nextchar() {
	if (inptr >= linelen) { // should be true on startup 
						  // TODO initscalars
		if (ch == EOF) {
			//printf("EOF encountered, program incomplete");
			return;
		}
		getLine();
	}
	ch = inbuff[ inptr];
	++ inptr;
}

void skipsep() {
	while (inSeparators( ch)) {
		nextchar();
	}
}

void clrident() {
	int i;
	for ( i = 0; i < idbuffsize; ++ i)
		idbuff[ i] = '\0';
}

void bldident() {

	clrident();

	int k = 0; // number of chars in the ident
	do {
		if (k < idbuffsize) {
			idbuff[ k] = ch;
			++ k;
		}
		nextchar();
	} while (( inDigits(ch) || inLetters(ch)));

	if (k >= idlen)
		idlen = k;
	else { // new ident shorter than old ident so rewrite end of buffer
		do {
			idbuff[ idlen] = '\0'; // write blanks over leftover chars
			-- idlen;
		} while (idlen != k);
	}	

	for ( k = 0; k < idlen; ++ k) { // convert to lowercase
		if (inUppercase(idbuff[ k]))
			idbuff[ k] += 'a' - 'A';
	}
}

void scanident() {
	// ident -> letter { [ underscore ] ( letter | digit ) }
	// letter -> 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i' | 'j'
	//                | 'k' | 'l' | 'm' | 'n' | 'o' | 'p' | 'q' | 'r' | 's' 
	//                | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z'

	bldident();
	bool isResWrd = false;

	int i;
	for ( i = 0; i < nreswrd && !isResWrd; ++ i) { // linear search of reswords
		if (strcmp(reswrd[i], idbuff) == 0) {
			isResWrd = true;
			// since i is updated at the end of the loop
			// decrement here since when ++i will get to actual value
			-- i;
		}
	}

	if (isResWrd)
		sym = reswrdsym[ i];
	else
		sym = ident;
}

// void bldint() { // should scan digits of int and convert to binary
// 				// unsignedint -> digit { digit }
// 	int digval;

// 	intval = 0;

// 	while( ch == '0') // ignore leading zeroes
// 		nextchar();
// 	while( inDigits( ch)) {
// 		digval = (int) (ch - '0');

// 		if (intval <= (intmax - digval)/10) 
// 			intval = 10*intval + digval;
// 			nextchar();
// 	}
// }

int rebuffHex( int dec) { // returns index to keep adding to
	int i, j;
	char temp[ 256];
	for( i = 0; i < 256; ++ i) {
		hexBuff[ i] = '\0';
		temp[ i] = '\0';
	}

	i = 0;
	do {
		int curDig = dec%10;
		temp[ i] = (char) (curDig + (int)('0'));

		dec = dec/10;
		++i;
	} while( dec != 0);

	// now need to reverse the array
	for( j = i - 1; j >= 0; -- j) 
		hexBuff[ (i - 1) - j] = temp[ j];

	return i;
}

void scannum() {
	sym = INTEGER_SYM;

	//bldint();

	int digval;

	intval = 0;

	while( ch == '0') // ignore leading zeroes
		nextchar();
	while( inDigits( ch)) {
		digval = (int) (ch - '0');

		intval = 10*intval + digval;
		nextchar();
	}

	if ( ch == '.') {
		// look at next char but don't actually process it
		if ( inptr >= linelen) {
			// then . was the last char on the line and it's broken
			error( 10);
			return;
		}
		// ok, then the . wasn't the last char
		// the next char is inbuff[ inptr]
		char lookahead = inbuff[ inptr];
		if( lookahead == '.') {
			// then it's a range
			return;
		} 
		else if( inDigits( lookahead)) {
			numdigs = -1; // it'll get incremented one too many times
			decimals = 0;
			sym = REAL_SYM;
			nextchar(); // to eat the .
			while( inDigits( ch)) {
				digval = (int) (ch - '0');

				decimals = 10*decimals + digval;
				++ numdigs;
				nextchar();
			}	
		} 
		else if( inSeparators( lookahead)) { // assume num. is num.0
			numdigs = 0;
			decimals = 0;
			sym = REAL_SYM;
			nextchar();
		}
		else {
			error( 10); // error in number format
		}
	}
	else if( inHexDigits( ch)) {
		sym = HEXINT_SYM;
		// hex numbers are dig dig* hex hex* H
		// works b/c decimals digs count as hex

		// first convert the int you have from hex to dec, since we're storing in dec
		// actually i take it back, let's store hex numbers as 
		// char arrays, then we can print them as hex for output
		int hexInd = rebuffHex( intval); 

		// then, hexInd stores the end of the current int so you can 
		// keep adding to the buffer
		while( inHexDigits( ch)) {
			hexBuff[ hexInd] = ch;
			++ hexInd;
			nextchar();
		}

		if( ch != 'H') 
			error( 16); // something wrong with the hex
		else 
			nextchar(); // get rid of the H
	} else if( ch == 'H') {
		// then it was a hex number with no letter parts
		rebuffHex( intval);
		sym = HEXINT_SYM;
		nextchar();
	}

}

void rcomment() {
	// comments in oberon are (* ..... *)
	// also there can be nested comments 

	// start this method when (*  already found
	char temp; // nextchar was already called so on first char in comment
	int clev = 0; // comment level 0 is first level
	while( true) {
		temp = ch;
		nextchar();

		if (temp == '(' && ch == '*') {
			++ clev;
			nextchar();
		}
		if (temp == '*' && ch == ')') {
			-- clev;
			if (clev < 0)
				break;
		}

		if( ch == EOF) { // uh oh
			error( 45);
			exit( 0);
		}
	}
	nextchar();
}

void idassign() {
	sym = colon;
	nextchar();
	if ( ch == '=') {
		sym = assign;
		nextchar();
	}
}

void idrelop() { // this will be >= or <=
	sym = spsym[ ch];
	nextchar(); 
	if ( sym == lt && ch == '=') {
		sym = lte;
		nextchar();
	}
	else if (sym == gt && ch == '=') {
		sym = gte;
		nextchar();
	}
}

float expo(float base, int exp) {
	float res = base;
	int i;
	for ( i = 0; i < exp; ++ i)
		res *= base;
	return res;
}

void writesym() {
	fputs( symname[ sym][ 0], stdout);

	switch( sym) {
		case ident:
			fputs( ": ", stdout);
			fputs( idbuff, stdout);
			break;
		case CHAR_SYM:
		case string:
			fputs( ": ", stdout);
			fputs( strbuff, stdout);
			break;
		case INTEGER_SYM: 
			printf( ": %d", intval);
			break;
		case HEXINT_SYM: 
			fputs(": ", stdout);
			fputs( hexBuff, stdout);
			break;
		case REAL_SYM:
			printf( ": %f", (intval + decimals/(expo(10.0f, numdigs))));
			break;
		default:
			break;
	}
	
	fputs( "\n", stdout);
}

void scanstr() {
	strlength = 0;
	do {
		do {
			nextchar();
			char temp = ' ';

			if ( ch == '\\') {
				temp = '\\';
				nextchar();
			}

			if ( ch == EOF) {
				error( 50); // missing "" for the string
				exit( 0); // end the program
			}

			if ( temp == '\\') { // don't get rid of actual metachars
				if ( ch == 't') 
					ch = '\t';
				else if ( ch == 'n')
					ch = '\n';
				else if ( ch == '\r')
					ch = '\r';
			}

			strbuff[ strlength] = ch;
			++ strlength;

			if (temp == '\\' && ch == '"') // not end of string
				ch = ' ';

		} while( ch != '"');
		nextchar();
	} while( ch == '"');

	strbuff[ --strlength] = '\0'; // null character to end the string
								  // also last char is not in the string (it's the quote)
	if (strlength == 1)
		sym = CHAR_SYM; // a single char is a char instead of a string
	else
		sym = string;
}

void nextsym() {
	// continue until symbol found
	bool symfnd;
	do {
		symfnd = true;
		skipsep();
		symptr = inptr;

		if (inLetters( ch)) { // ident or reserved word
			scanident();
		}
		else {
			if (inDigits( ch)) // number
				scannum();
			else {
				switch( ch) {
					case '"':
						scanstr();
						break;
					case '.':
						sym = per;
						nextchar();
						if ( ch == '.') {
							sym = doubledot;
							nextchar();
						}
						break;
					case ':':
						idassign();
						break;
					case '<': // fall through to case '>', both call relop
					case '>':
						idrelop();
						break;
					case '(': // check for comment
						nextchar();
						if ( ch != '*')
							sym = lparen;
						else { // now we know it's a comment
							symfnd = false;
							nextchar();
							rcomment();
						}
						break;
					default: // then should be in one of the "special" chars (mul, slash, etc.)
						sym = spsym[ ch];
						nextchar();
						break;
				} // end switch
			}
		}
	} while (!symfnd);
}

int main( int argc, char **argv) {
   ch = ' ';
   InitCompile();

   // arg[1] is the first command line arg
   
   fileIn = fopen( argv[ 1], "r");

   // while (ch != EOF) {
   // 		nextchar();
   // 		putc( ch, stdout);
   // 		printf("  ");
   // }

   //fputs( reswrd[ 0][0], stdout);

   while ( ch != EOF) {
	   nextsym();
	   if ( ch != EOF)
	   	writesym();
	}

	// int dec = 1234;
	// rebuffHex(dec);

	// fputs( hexBuff, stdout);
	

   // if (inUppercase('C') && inUppercase('A') && inUppercase('Z') && inLowecase('b') && inLowecase('z') && !inUppercase('9') && !inLowecase('.'))
   // 	printf("HERE");
   
   return(0);
}
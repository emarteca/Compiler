#include <stdio.h>
#include <stdlib.h> // just using for the exit function


/*
  Oberon scanner 
  Compile:    gcc scanner.c
  Run:        ./a.out fileToScan

  Ellen Arteca (0297932)
*/


typedef enum { false, true } bool;  // since bool isn't a type in C


const int nreswrd = 41;       // number of reserved words
const int inbuffsize = 256;   // input buffer (line) size
const int idbuffsize = 16;    // ident buffer size


typedef enum 
{
  BOOLEAN_SYM, CHAR_SYM, FALSE_SYM, TRUE_SYM, NEW_SYM,
  real_number, REAL_SYM, INTEGER_SYM, int_number, hexint_number, ARRAY_SYM, IMPORT_SYM, THEN_SYM, BEGIN_SYM, IN_SYM,
  TO_SYM, BY_SYM, IS_SYM, CASE_SYM, MOD_SYM, TYPE_SYM, CONST_SYM,
  MODULE_SYM, UNTIL_SYM, DIV_SYM, NIL_SYM, VAR_SYM, DO_SYM, OF_SYM,
  WHILE_SYM, ELSE_SYM, OR_SYM, ELSIF_SYM, POINTER_SYM, END_SYM, 
  PROCEDURE_SYM, RECORD_SYM, FOR_SYM, REPEAT_SYM, IF_SYM, RETURN_SYM,
  EXIT_SYM, LOOP_SYM, WITH_SYM,

  lparen, rparen, plus, hyphen, star, slash, rbrac, lbrac, equal, colon,
  lt, lte, gt, gte, semic, null, assign, carat, neq, comma, per, ident, 
  number, string, eof_sym, invalid_sym, op_sym, set_sym, tilde, 
  lcurb, rcurb, resword, doubledot  
} Token;

const char *reswrd[ 41][ 50];        // array of reserved words
const char *symname[ 127][ 50];      // array of symbol names
const char *errmsg[ 256][ 32];       // 256 is errmax 
Token reswrdsym[ 41];
Token spsym[ 127]; 


FILE* fileIn;       // file reading in from
char ch;            // current char
Token sym;          // current symbol
char inbuff[ 256];  // inbuffsize
char idbuff[ 16];   // idbuffsize
char strbuff[ 80];  // max size for string

int intval;         // value of current int being read
int decimals;       // any numbers past the decimal point (if real)
int numdigs;        // number of decimal digits

char hexBuff[ 256]; // let's make the hex numbers have a limit of 256 digs

int inptr;      // pointer to current char in inbuff
int symptr;         // pointer to current symbol in inbuff
int linelen = 0;    // number of chars on current line
int linenum = 0;    
int idlen = 16;     // idbuffsize
int strlength = 0;
char strToMatch = '\'';    // character to match to end the string (this lets you have embedded quotes if they're the opposite type)
						   // so, for example, you have have "I'm hungry"  or 'Go buy some "food" from aramark'

// initialize error msgs 
void InitErrMsgs() 
{
  int i;
  for( i = 0; i < 256; ++ i) 
    errmsg[ i][ 0] = "\0";

  errmsg[ 10][ 0] = "Error in general number format";
  errmsg[ 16][ 0] = "Error in hex number formatting";
  errmsg[ 30][ 0] = "Number too large";
  errmsg[ 39][ 0] = "39";
  errmsg[ 45][ 0] = "Unfinished comment... EOF reached";
  errmsg[ 50][ 0] = "String delimiter missing";
} // end InitErrMsgs

// initialize special syms (one-char syms)
void InitSpSyms() // one-char symbols (ie. not <=, >=, etc.)
{ 
  int i;
  for ( i = 0; i < 127; ++ i)
    spsym[ i] = null;

  spsym[ '('] = lparen;
  spsym[ ')'] = rparen;
  spsym[ '+'] = plus;
  spsym[ '-'] = hyphen;
  spsym[ '*'] = star;
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
} // end InitSpSyms

// initialize the table of spellings of symbols, required since
// the symbols, being an enumerated type, cannot be written as such
void InitSymNames() 
{
  int i;
  for ( i = 0; i < 127; ++ i)
    symname[ i][ 0] = "\0"; // start by setting all entries to null string

  symname[   BOOLEAN_SYM][ 0] = "BOOLEAN_SYM";
  symname[      CHAR_SYM][ 0] = "CHAR_SYM";
  symname[     FALSE_SYM][ 0] = "FALSE_SYM";
  symname[      TRUE_SYM][ 0] = "TRUE_SYM";
  symname[       NEW_SYM][ 0] = "NEW_SYM";
  symname[   real_number][ 0] = "real_number";
  symname[    int_number][ 0] = "int_number";
  symname[      REAL_SYM][ 0] = "REAL_SYM";
  symname[   INTEGER_SYM][ 0] = "INTEGER_SYM";
  symname[ hexint_number][ 0] = "hexint_number";
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
  symname[        hyphen][ 0] = "hyphen";
  symname[          star][ 0] = "star";
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
} // end InitSymNames

// initialize reserved words 
void InitResWrds() 
{
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
} // end InitResWrds

// initialize reserved word symbols (from Token enum above)
void InitResWrdSyms() 
{
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
} // end InitResWrdSyms

// call all methods necessary to set up the scanner 
void InitCompile() 
{
  InitResWrds();
  InitResWrdSyms();
  InitSpSyms();
  InitSymNames();
  InitErrMsgs();
} // end InitCompile

// method to print the error message corresponding to the index 
// of eNum in the array errmsg
void error( int eNum) 
{
  fputs( "------------------------------------\n", stdout);
  fputs( errmsg[ eNum][ 0], stdout);
  fputs( "\n------------------------------------\n", stdout);
  // sym = invalid_sym;
} // end error

// return true if c is a separator
// (whitespace, horizontal tab, newline, carriage return)
bool inSeparators( char c) 
{
  if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
    return true;
  return false;
}

// return true if c is a lowercase letter
bool inLowecase( char c) 
{
  return c >= 'a' && c <= 'z';
}

// return true if c is an uppercase letter
bool inUppercase( char c) 
{
  return c >= 'A' && c <= 'Z';
}

// return true if c is a letter (upper or lowercase)
bool inLetters( char c) 
{
  return inLowecase(c) || inUppercase(c);
}

// return true if c is a digit (number in [0, 9])
bool inDigits( char c) 
{
  return c >= '0' && c <= '9';
}

// return true if c is a hex digit
// this includes numbers in [0, 9], and letters in [A, F]
bool inHexDigits( char c) 
{
  return (c >= 'A' && c <= 'F') || inDigits( c);
}

// read the next line from the src file into inbuff
void getLine() 
{
  ++ linenum;

  inptr = 0;

  char temp = getc( fileIn);
  inbuff[ inptr] = temp;

  // read in the next line
  while (temp != '\n' && temp != EOF && temp != '\0') 
  {
    temp = getc( fileIn);
    ++ inptr;
    inbuff[ inptr] = temp;
  } // end while

  ++ inptr;
  inbuff[ inptr] = '\n';

  linelen = inptr;

  if (inptr == 1) // empty line has a length of 0
    linelen = 0;

  inptr = 0;

  ch = temp; // last char read in
} // end getLine

// gets the next character from the input buffer
void nextchar() 
{

  if (inptr >= linelen)  // should be true on startup 
  {
              
    if (ch == EOF) 
    {
      //printf("EOF encountered, program incomplete");
      return;
    } // end EOF check if
    getLine();
  } // end inptr check if

  ch = inbuff[ inptr];
  ++ inptr; // move inptr to point to the next char in the input buffer
} // end nextchar

// skip separators at the beginning of the line
// recall separators are whitespace, \n, \t, \r
void skipsep() 
{
  while (inSeparators( ch)) 
  {
    nextchar();
  }
}

// build identifier
// this method is called when it's already an ident 
// so, char (char | dig)*
void bldident() 
{

  // clear old ident
  int k;
  for ( k = 0; k < idbuffsize; ++ k)
    idbuff[ k] = '\0';

  k = 0; // number of chars in the ident

  do // read in chars into ident while they're numbers or letters 
  { 
    if (k < idbuffsize) 
    {
      idbuff[ k] = ch;
      ++ k;
    } // end if
    nextchar();
  } while (( inDigits(ch) || inLetters(ch)));

  if (k >= idlen)
    idlen = k;
  else                       // new ident shorter than old ident so rewrite end of buffer
  { 
    do 
    {
      idbuff[ idlen] = '\0'; // write blanks over leftover chars
      -- idlen;
    } while (idlen != k);
  }  // end else

  // don't convert to lowercase b/c Oberon is case sensitive!!!

} // end bldident

// compare 2 strings 
int strcmp( const char *a, const char *b) 
{
  // we're going with a is uppercase
  // and b is lowercase
  int i;
  for( i = 0; a[ i] != '\0' && b[ i] != '\0';) 
  {
    if( a[ i] != b[ i]  && a[ i] != '\0' && b[ i] != '\0')
      return 1; // not same
    ++i;
    if( ( a[ i] == '\0' && b[ i] != '\0') || ( b[ i] == '\0' && a[ i] != '\0'))
      return 1; // not same
  } // end for

  return 0; // equal word
} // end strcmp

// build ident and figure out if it's a reserved word or just normal ident
void scanident() 
{
  // ident -> letter { [ underscore ] ( letter | digit ) }
  // letter -> 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i' | 'j'
  //                | 'k' | 'l' | 'm' | 'n' | 'o' | 'p' | 'q' | 'r' | 's' 
  //                | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z'

  bldident();
  bool isResWrd = false;

  int i;
  for ( i = 0; i < nreswrd && !isResWrd; ++ i) // linear search of reswords
  { 
    if (strcmp( reswrd[ i][ 0], idbuff) == 0) 
    {
      isResWrd = true;
      // since i is updated at the end of the loop
      // decrement here since when ++i will get to actual value
      -- i;
    }
  }

  if ( isResWrd)
    sym = reswrdsym[ i];
  else
    sym = ident;
} // end scanident


// hex number is stored in a buffer
// this takes the current int and puts it in the hex buffer
// mostly for hex numbers starting with ints which were initially read 
// into ints
int rebuffHex( int dec) // returns index to keep adding to
{ 
  int i, j;
  char temp[ 256];
  // empty buffer
  for( i = 0; i < 256; ++ i) 
  {
    hexBuff[ i] = '\0';
    temp[ i] = '\0';
  }

  i = 0;
  do                // go digit per digit and store in hex buffer
  {   
    int curDig = dec%10;
    temp[ i] = (char) (curDig + (int)('0'));

    dec = dec/10;
    ++i;
  } while( dec != 0);

  // now need to reverse the array
  for( j = i - 1; j >= 0; -- j) 
    hexBuff[ (i - 1) - j] = temp[ j];

  return i;
} // end rebuffHex

// computes basse to the power of exp
// where exp is a positive integer
float expo(float base, int exp) 
{
  float res = 1;
  int i;
  for ( i = 0; i < exp; ++ i)
    res *= base;
  return res;
} // end expo

int getValue() // return value of current hex in decimal
{
  int i = 255; // last index of char
  int curPow = 0;
  int totalVal = 0;
  
  while ( i >= 0)
  {
  	if ( hexBuff[ i] != '\0')
  	{
      int curDig = 0;
  	  char curChar = hexBuff[ i];

  	  if ( inUppercase( curChar))        // then it's an uppercase letter from A to F inclusive
  	  	curDig = 10 + (int) ( curChar - 'A');
  	  else                               // then it's a number
  	    curDig = (int) ( curChar - '0');

  	  totalVal += curDig * expo( 16, curPow);
  	  ++ curPow;
  	}
  	-- i;
  }
  return totalVal;
}

// scan a number (int/hex/real)
void scannum() 
{
  sym = int_number;

  int digval;

  intval = 0;

  while( ch == '0') // ignore leading zeroes
    nextchar();
  while( inDigits( ch)) 
  {
    digval = (int) (ch - '0');

    intval = 10*intval + digval;
    nextchar();
  } // end while

  if ( ch == '.') 
  {
    // look at next char but don't actually process it
    if ( inptr >= linelen) 
    {
      // then . was the last char on the line and it's broken
      error( 10);
      return;
    }
    // ok, then the . wasn't the last char
    // the next char is inbuff[ inptr]
    char lookahead = inbuff[ inptr];
    if( lookahead == '.') 
    {
      // then it's a range
      return;
    } 
    else if( inDigits( lookahead)) 
    {
      numdigs = 0; 
      decimals = 0;
      sym = real_number;
      nextchar(); // to eat the .
      while( inDigits( ch)) 
      {
        digval = (int) (ch - '0');

        decimals = 10*decimals + digval;
        ++ numdigs;
        nextchar();
      }  // end while
    } 
    else if( inSeparators( lookahead)) // assume num. is num.0
    { 
      numdigs = 0;
      decimals = 0;
      sym = real_number;
      nextchar();
    }
    else 
    {
      error( 10); // error in number format
    }
  } // end if for lookahead being a digit
  else if( inHexDigits( ch)) 
  {
    sym = hexint_number;
    // hex numbers are dig dig* hex hex* H
    // works b/c decimals digs count as hex

    // first convert the int you have from hex to dec, since we're storing in dec
    // actually i take it back, let's store hex numbers as 
    // char arrays, then we can print them as hex for output
    int hexInd = rebuffHex( intval); 

    // then, hexInd stores the end of the current int so you can 
    // keep adding to the buffer
    while( inHexDigits( ch)) 
    {
      hexBuff[ hexInd] = ch;
      ++ hexInd;
      nextchar();
    }

    if ( ch == 'H')
      nextchar(); // get rid of the H
    else if ( ch == 'X') 
    {
      // it was a one-character literal specified by its ordinal number (number between 0 and 255 in hex)
      // need to check the range of the number
      int ord = getValue();
      if ( ord < 0 || ord > 255)
        error( 16); // error in hex
      sym = CHAR_SYM;

      strlength = 1;
      strbuff[ 0] = (char) ord;
      strbuff[ 1] = '\0';

      nextchar();

    }
    else 
      error( 16); // something wrong with the hex
    
  } 
  else if( ch == 'H') 
  {
    // then it was a hex number with no letter parts
    rebuffHex( intval);
    sym = hexint_number;
    nextchar();
  } 
  else if ( ch == 'X')
  {
  	// then, it was a one-character literal specified by its ordinal number (number between 0 and 255 in hex)
  	rebuffHex( intval);
  	// need to check the range of the number
  	int ord = getValue();
  	if ( ord < 0 || ord > 255)
      error( 16); // error in hex
  	sym = CHAR_SYM;

  	strlength = 1;
    strbuff[ 0] = (char) ord;
    strbuff[ 1] = '\0';

    nextchar();

  }

} // end scannum


// scan comments 
// they are effectively ignored i.e. not included in the parse output
void rcomment() 
{
  // comments in oberon are (* ..... *)
  // also there can be nested comments 

  // start this method when (*  already found
  char temp; // nextchar was already called so on first char in comment
  int clev = 0; // comment level 0 is first level
  while( true) 
  {
    temp = ch;
    nextchar();

    if (temp == '(' && ch == '*') 
    {
      ++ clev;
      nextchar();
    }
    if (temp == '*' && ch == ')') 
    {
      -- clev;
      if (clev < 0)
        break;
    }

    if( ch == EOF)  // uh oh
    { 
      error( 45);
      exit( 0);
    }
  }
  nextchar();
} // end rcomment

// check if assignment statement :=
void idassign() 
{
  sym = colon;
  nextchar();
  if ( ch == '=') 
  {
    sym = assign;
    nextchar();
  }
} // end idassign

// check if relational operator with 2 chars
// this will be >= or <=
void idrelop() { 
  sym = spsym[ ch];
  nextchar(); 
  if ( sym == lt && ch == '=') 
  {
    sym = lte;
    nextchar();
  }
  else if (sym == gt && ch == '=') 
  {
    sym = gte;
    nextchar();
  }
} // end idrelop

// method to print the current symbol 
// mostly for debugging
void writesym() 
{
  fputs( symname[ sym][ 0], stdout);

  switch( sym) 
  {
    case ident:
      fputs( ": ", stdout);
      fputs( idbuff, stdout);
      break;
    case CHAR_SYM:
    case string:
      fputs( ": ", stdout);
      fputs( strbuff, stdout);
      break;
    case int_number: 
      printf( ": %d", intval);
      break;
    case hexint_number: 
      fputs(": ", stdout);
      fputs( hexBuff, stdout);
      break;
    case real_number:
      printf( ": %f", (intval + decimals/(expo(10.0f, numdigs))));
      break;
    default:
      break;
  } // end switch
  
  fputs( "\n", stdout);
} // end writesym

// scan a string " char* "   or ' char* '
void scanstr() 
{
  strlength = 0;
  do 
  {
    do 
    {
      nextchar();
      char temp = ' ';

      if ( ch == '\\') 
      {
        temp = '\\';
        nextchar();
      }

      if ( ch == EOF) 
      {
        error( 50); // missing "" for the string
        exit( 0); // end the program
      }

      if ( temp == '\\') // don't get rid of actual metachars
      { 
        if ( ch == 't') 
          ch = '\t';
        else if ( ch == 'n')
          ch = '\n';
        else if ( ch == '\r')
          ch = '\r';
      }

      strbuff[ strlength] = ch;
      ++ strlength;

      if (temp == '\\' && ch == strToMatch) // not end of string
        ch = ' ';

    } while( ch != strToMatch);
    nextchar();
  } while( ch == strToMatch);

  strbuff[ --strlength] = '\0'; // null character to end the string
                  // also last char is not in the string (it's the quote)
  if (strlength == 1)
    sym = CHAR_SYM; // a single char is a char instead of a string
  else
    sym = string;
} // end scanstr

// scan the next symbol
void nextsym() 
{
  // continue until symbol found
  bool symfnd;
  do 
  {
    symfnd = true;
    skipsep();
    symptr = inptr;

    if (inLetters( ch)) // ident or reserved word
    { 
      scanident();
    }
    else 
    {
      if (inDigits( ch)) // number
        scannum();
      else 
      {
        switch( ch) 
        {
          case '"':
          	strToMatch = '"';
          	scanstr();
            break;
          case '\'': 
          	strToMatch = '\'';
            scanstr();
            break;
          case '.':
            sym = per;
            nextchar();
            if ( ch == '.') 
            {
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
            else // now we know it's a comment
            { 
              symfnd = false;
              nextchar();
              rcomment();
            }
            break;
          case EOF:
            sym = eof_sym;
            break;
          default: // then should be in one of the "special" chars (star, slash, etc.)
            sym = spsym[ ch];
            nextchar();
            break;
        } // end switch
      }   // end if
    }     // end do
  } while (!symfnd);
} // end nextsym


// run the code
int main( int argc, char **argv) 
{
   ch = ' ';
   InitCompile();

   // arg[1] is the first command line arg

   if ( argc != 2) 
   {
       fputs( "\nError exiting now\nUsage: ./a.out fileToScan\n\n", stdout);
       exit( 0);
   }
   
   fileIn = fopen( argv[ 1], "r");

   do 
   {
     nextsym();
     writesym();
    } while( sym != eof_sym); 

   return(0);
} // end main
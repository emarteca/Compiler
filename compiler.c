#include <stdio.h>
#include <stdlib.h> // just using for the exit function
#include "parser.h"


/*
  Oberon scanner + parser + code gen
  Compile:    gcc compiler.c
  Run:        ./a.out fileToCodeGen

  Ellen Arteca (0297932)
*/

typedef enum { false, true } bool;  // since bool isn't a type in C

bool debugMode = true;

const int nreswrd = 41;       // number of reserved words
const int inbuffsize = 256;   // input buffer (line) size
const int idbuffsize = 16;    // ident buffer size


typedef enum 
{
  BOOLEAN_SYM, CHAR_SYM, FALSE_SYM, TRUE_SYM, NEW_SYM,
  real_number, REAL_SYM, INTEGER_SYM, int_number, hexint_number, ARRAY_SYM, IMPORT_SYM, THEN_SYM, BEGIN_SYM, IN_SYM,
  TO_SYM, BY_SYM, IS_SYM, CASE_SYM, MOD_SYM, TYPE_SYM, CONST_SYM,
  MODULE_SYM, UNTIL_SYM, DIV_SYM, NIL_SYM, VAR_SYM, DO_SYM, OF_SYM,
  WHILE_SYM, ELSE_SYM, OR_SYM, AND_SYM, ELSIF_SYM, POINTER_SYM, END_SYM, 
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


union conststruct // double or int
{ 
  int i;
  double r;
};

struct typstruct 
{
  // empty
};

struct varstruct 
{
  int varaddr;
}; 

struct procstruct 
{
  int paddr;
  int lastparam;
  int resultaddr;
};

struct paramstruct 
{
  int varparam;
  int paramaddr;
};

struct stdpstruct
{
  int procnum;
};

union classdata 
{
  struct typstruct t;
  struct varstruct v;
  struct procstruct p;
  struct paramstruct pa;
  struct stdpstruct sp;
  union conststruct c;
}; 

typedef enum 
{
  constcls, typcls, varcls, procls, paramcls, stdpcls
} idclass;  

struct identrec 
{
  char idname[ 25]; // identifier spelling (the name)
  int previd;
  int idlev;
  int idtyp;
  idclass Class;
  union classdata data;
};

typedef enum 
{
  scalarfrm,
  arrayfrm,
  recordfrm,
  ptrfrm,
  procfrm,
} typform;

struct typrec
{
  int size;
  typform form;
};

struct identrec symtab[ 256]; // symbol table
const int stsize = 256;
struct typrec typtab[ 128];   // type table
const int ttsize = 128;
int scoptab[ 10];             // scope table

int stptr = 0; 
int ttptr = 0;
int currlev = 0; // current scope level
int maxlev = 10; // max scope level

int inttyp, realtyp, booltyp, chartyp;


// method to print the error message corresponding to the index 
// of eNum in the array errmsg
void error( Token expSym, int eNum) 
{
  fputs( "------------------------------------\n", stdout);
  fputs( errmsg[ eNum][ 0], stdout);
  fputs( "\n------------------------------------\n", stdout);
  // sym = invalid_sym;
} // end error


void strcopy( char *toCopy, char* copyTo)
{
  // the end point is probably the null char
  int i = 0;
  while( toCopy[ i] != '\0')
  {
    copyTo[ i] = toCopy[ i];
    ++i;
  }
}


void EnterScop()
{
  if ( currlev < maxlev)
  {
    ++ currlev;
    scoptab[ currlev] = 0; // create entry for new scope level
  }
  else
  {
    printf( "FATAL ERROR: max scope level reached");
    exit( 0);
  }
}

void ExitScop()
{
  -- currlev;
}



void enterstdtypes()
{
  // int
  ++ ttptr;
  inttyp = ttptr;
  typtab[ ttptr].size = 1;
  typtab[ ttptr].form = scalarfrm;

  // real
  ++ ttptr;
  realtyp = ttptr;
  typtab[ ttptr].size = 1;
  typtab[ ttptr].form = scalarfrm;

  // boolean
  ++ ttptr;
  booltyp = ttptr;
  typtab[ ttptr].size = 1;
  typtab[ ttptr].form = scalarfrm;

  // char
  ++ ttptr;
  chartyp = ttptr;
  typtab[ ttptr].size = 1;
  typtab[ ttptr].form = scalarfrm;

}

void printsymtab()
{
  printf( "\n");
  printf( "\nName\tLevel\tType\tPrevid\tAddr");
  printf( "\n");
  int i;
  for( i = 1; i < stptr + 1; ++ i)
  {
    printf( "%s", symtab[ i].idname);
    printf( "\t%d", symtab[ i].idlev);
    printf( "\t%d", symtab[ i].idtyp);
    printf( "\t%d", symtab[ i].previd);

    // case for type TODO

    printf( "\n");
  }

  printf( "\n");
  // scoptab TODO
}

void enterstdident( char* id, idclass cls, int ttp)
{
  ++ stptr;
  strcopy( id, symtab[ stptr].idname);
  symtab[ stptr].previd = scoptab[ currlev];
  symtab[ stptr].idlev = currlev;
  symtab[ stptr].idtyp = ttp;
  symtab[ stptr].Class = cls;

  scoptab[ currlev] = stptr;

  printsymtab();
}

void LookupId( char* id, int *stp)
{
  int lev;
  strcopy( id, symtab[ 0].idname);
  lev = currlev;

  do {
    *stp = scoptab[ lev];
    while( strcmp( symtab[ *stp].idname, id) == 0)
    {
      *stp = symtab[ *stp].previd;
    }
    -- lev;
  } while( *stp == 0 && lev >= 0);

  if( *stp == 0)
  {
    error( invalid_sym, 11);
  }
}

void InsertId( char* id, idclass cls)
{
  int stp;
  strcopy( id, symtab[ 0].idname); // sentinel for search
  stp = scoptab[ currlev]; // top of symtab for current scope
  printf( "Here!!!!!!!!!!!!!!!!!!!!!!!!!!%d", stptr);
  while( strcmp( symtab[ stp].idname, id) == 0) // if words are equal
  {
    stp = symtab[ stp].previd; // searching current scope
  }
  if( stp == 0)
  {
    error( invalid_sym, 12); // multiply declared ident
  }

  if( stptr < stsize)
  {
    ++ stptr;
  }
  else
  {
    printf( "Symbol table overflow... exiting now");
    exit( 0);
  }

  strcopy( id, symtab[ stptr].idname);
  symtab[ stptr].Class = cls;
  symtab[ stptr].idlev = currlev;
  symtab[ stptr].previd = scoptab[ currlev];

  scoptab[ currlev] = stptr;

  printsymtab();
}

void InitSymTab()
{
  scoptab[ currlev] = 0; // currlev is already 0

  // there are 27 stdidents in Oberon (ostensibly)
  char *stdidents[ 27][ 25];

  stdidents[  0][ 0] = "ABS";
  stdidents[  1][ 0] = "CHAR";
  stdidents[  2][ 0] = "FLT";
  stdidents[  3][ 0] = "LSL";
  stdidents[  4][ 0] = "REAL";
  stdidents[  5][ 0] = "ASR";
  stdidents[  6][ 0] = "CHR";
  stdidents[  7][ 0] = "INC";
  stdidents[  8][ 0] = "NEW";
  stdidents[  9][ 0] = "ROR";
  stdidents[ 10][ 0] = "ASSERT";
  stdidents[ 11][ 0] = "DEC";
  stdidents[ 12][ 0] = "INCL";
  stdidents[ 13][ 0] = "ODD";
  stdidents[ 14][ 0] = "SET";
  stdidents[ 15][ 0] = "BOOLEAN";
  stdidents[ 16][ 0] = "EXCL";
  stdidents[ 17][ 0] = "INTEGER";
  stdidents[ 18][ 0] = "ORD";
  stdidents[ 19][ 0] = "UNPK";
  stdidents[ 20][ 0] = "BYTE";
  stdidents[ 21][ 0] = "FLOOR";
  stdidents[ 22][ 0] = "LEN";
  stdidents[ 23][ 0] = "PACK";
  stdidents[ 24][ 0] = "Out.Int";
  stdidents[ 25][ 0] = "Out.Ln";
  stdidents[ 26][ 0] = "In.Int";

  enterstdtypes();

  enterstdident( stdidents[ 17][ 0], typcls, inttyp);     // INTEGER
  enterstdident( stdidents[  4][ 0], typcls, realtyp);    // REAL
  enterstdident( stdidents[  1][ 0], typcls, chartyp);    // CHAR
  enterstdident( stdidents[ 15][ 0], typcls, booltyp);    // BOOLEAN
  enterstdident( stdidents[ 24][ 0], typcls, 0);          // Out.Int
  enterstdident( stdidents[ 25][ 0], typcls, 0);          // Out.Ln
  enterstdident( stdidents[ 26][ 0], typcls, 0);          // In.Int

  // std function calls (built in)
  enterstdident( stdidents[  0][ 0], stdpcls, 0);         // ABS
  symtab[ stptr].data.sp.procnum = 0;
  enterstdident( stdidents[ 13][ 0], stdpcls, 0);         // ODD  
  symtab[ stptr].data.sp.procnum = 1;

}

// initialize error msgs 
void InitErrMsgs() 
{
  int i;
  for( i = 0; i < 256; ++ i) 
    errmsg[ i][ 0] = "\0";

  errmsg[   1][ 0] = "ERROR 1";
  errmsg[  10][ 0] = "Error in general number format";
  errmsg[  16][ 0] = "Error in hex number formatting";
  errmsg[  30][ 0] = "Number too large";
  errmsg[  39][ 0] = "39";
  errmsg[  45][ 0] = "Unfinished comment... EOF reached";
  errmsg[  50][ 0] = "String delimiter missing";
  errmsg[ 122][ 0] = "Number expected";
  errmsg[ 123][ 0] = "= expected";
  errmsg[ 124][ 0] = "identifier expected";
  errmsg[ 125][ 0] = "semic or comma expected";
  errmsg[ 127][ 0] = "statement expected";
  errmsg[ 129][ 0] = "period expected";
  errmsg[ 131][ 0] = "undeclared ident";
  errmsg[ 133][ 0] = ":= expected";
  errmsg[ 136][ 0] = "THEN expected";
  errmsg[ 137][ 0] = "Semicolon or END expected";
  errmsg[ 138][ 0] = "DO expected";
  errmsg[ 140][ 0] = "Relational operator expected";
  errmsg[ 142][ 0] = ") expected";
  errmsg[ 143][ 0] = "( expected";
  errmsg[ 144][ 0] = "^ expected";
  errmsg[ 145][ 0] = "{ expected";
  errmsg[ 146][ 0] = "} expected";
  errmsg[ 147][ 0] = "] expected";
  errmsg[ 151][ 0] = "Semicolon expected";
  errmsg[ 153][ 0] = "colon expected";
  errmsg[ 154][ 0] = "MODULE expected";
  errmsg[ 155][ 0] = "END expected";
  errmsg[ 156][ 0] = "IMPORT expected";
  errmsg[ 157][ 0] = "ARRAY expected";
  errmsg[ 158][ 0] = "OF expected";
  errmsg[ 159][ 0] = "RECORD expected";
  errmsg[ 160][ 0] = "POINTER expected";
  errmsg[ 161][ 0] = "TO expected";
  errmsg[ 162][ 0] = "PROCEDURE expected";
  errmsg[ 163][ 0] = "IF expected";
  errmsg[ 164][ 0] = "CASE expected";
  errmsg[ 165][ 0] = "WHILE expected";
  errmsg[ 166][ 0] = "DO expected";
  errmsg[ 167][ 0] = "REPEAT expected";
  errmsg[ 168][ 0] = "UNTIL expected";
  errmsg[ 169][ 0] = "FOR expected";
  errmsg[ 170][ 0] = "LOOP expected";
  errmsg[ 171][ 0] = "EXIT expected";
  errmsg[ 172][ 0] = "WITH expected";
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
  spsym[ '&'] = AND_SYM;
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
  symname[       AND_SYM][ 0] = "AND_SYM";
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

  InitSymTab();
} // end InitCompile

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
  inbuff[ inptr + 1] = '\0';

  linelen = inptr;

  if (inptr == 1) // empty line has a length of 0
    linelen = 0;
  else
  {
    printf("\nCurrent line: %d: ", linenum);
    fputs( inbuff, stdout);
  }

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
      error( invalid_sym, 10); 
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
      error( invalid_sym, 10); // error in number format
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
        error( hexint_number, 16); // error in hex
      sym = CHAR_SYM;

      strlength = 1;
      strbuff[ 0] = (char) ord;
      strbuff[ 1] = '\0';

      nextchar();

    }
    else 
      error( hexint_number, 16); // something wrong with the hex
    
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
      error( hexint_number, 16); // error in hex
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
      error( invalid_sym, 45);
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
        error( string, 50); // missing "" for the string
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

  // TODO
  //writesym();

} // end nextsym





// *************************** END OF SCANNER
// *************************** STARTING PARSER





bool accept( Token expectedsym, int errnum) 
{
	if ( sym == expectedsym) 
	{
    writesym();
		nextsym();
		return true;
	}
	else 
	{
		error( expectedsym, errnum);
    printf("\n*********\nexpectedsym: ");
    fputs( symname[ expectedsym][ 0], stdout);
    printf("\nActual sym : ");
    writesym();
    printf("*********\n");
    nextsym();
		return false;
	}
}



bool isRelOp( Token sym1)
{
	/*
		relop -> = | # | < | <= | >= | IN | IS
	*/
	return ( sym1 == equal || sym1 == neq|| 
			  sym1 == lt || sym1 == lte || 
			  sym1 == gt || sym1 == gte ||
			  sym1 == IN_SYM || sym1 == IS_SYM);
}

bool isNumber( Token sym1)
{
	return ( sym1 == int_number || sym1 == hexint_number || sym1 == real_number);
}

bool isAddOp( Token sym1)
{
	/* 
		addop -> + | - | OR
	*/

	return ( sym1 == plus || sym1 == hyphen || sym1 == OR_SYM);
}

bool isMulOp( Token sym1)
{
	/*
		mulop -> * | / | DIV | MOD | &
	*/

	return ( sym1 == star || sym1 == slash || sym1 == DIV_SYM || sym1 == MOD_SYM || sym1 == AND_SYM);
}

bool inSelectorFirstSet( Token sym1)
{
	return ( sym1 == per || sym1 == lbrac || sym1 == carat); // || sym1 == lparen);
}

bool isLabel( Token sym1)
{
  /*
    label -> integer | string | ident
  */
  return ( sym1 == int_number || sym1 == string || sym1 == ident);
}


void module() 
{
	/*

		module -> MODULE ident ;
              [ ImportList ]
              DeclSeq
              [ BEGIN StatSeq ]
              END ident .
    */ 

    if ( debugMode) printf( "In Module\n");

    accept( MODULE_SYM, 154);
    accept( ident, 124);
    InsertId( idbuff, stdpcls);
    accept( semic, 151);

    EnterScop();

    if ( sym == IMPORT_SYM)
    {
    	ImportList();
    }

    DeclSeq();

    if ( sym == BEGIN_SYM)
    {
      writesym();
    	nextsym();
    	StatSeq();
    }

    ExitScop();

    accept( END_SYM, 155);
    accept( ident, 124);
    accept( per, 129);

    if ( debugMode) printf( "Out Module\n");
}

void ImportList()
{
	/*
		ImportList -> IMPORT import { , import } ;
	*/

  if ( debugMode) printf( "In ImportList\n");

	accept( IMPORT_SYM, 156);
	import();

	while ( sym == comma)
	{
    writesym();
		nextsym();
		import();
	}

	accept( semic, 151);

  if ( debugMode) printf( "Out ImportList\n");
}

void import() 
{
	/*
		import -> ident [ := ident ]
	*/

  if ( debugMode) printf( "In import\n");

	accept( ident, 124);
	if ( sym == assign)
	{
    writesym();
    nextsym();
		accept( ident, 124);
	}

  if ( debugMode) printf( "Out import\n");
}

void DeclSeq()
{
  /*
    DeclSeq -> { CONST { ConstDecl ; }
                 | TYPE { TypeDecl ; } 
                 | VAR { VarDecl ; } }
                 { ProcDecl ; | ForwardDecl ; }
  */

  if ( debugMode) printf( "In DeclSeq\n");

  while ( sym == CONST_SYM || sym == TYPE_SYM || sym == VAR_SYM)
  {
    writesym();
    switch( sym)
    {
      case CONST_SYM:
        nextsym();
        // start of ConstDecl is an ident
        while ( sym == ident)
        {
          ConstDecl();
          accept( semic, 151);
        }
        break;
      case TYPE_SYM:
        nextsym();
        while ( sym == ident)
        {
          TypeDecl();
          accept( semic, 151);
        }
        break;
      case VAR_SYM:
        nextsym();
        while ( sym == ident)
        {
          VarDecl();
          accept( semic, 151);
        }
    }
  }  

  while ( sym == PROCEDURE_SYM)
  {
    writesym();
    nextsym();
    if ( sym == carat) 
    {
      // then it's ForwardDecl
      writesym();
      nextsym();
      if ( sym != ident)
      {
        Receiver();
      }
      identdef( procls);
      if ( sym == lparen)
      {
        FormParams();
      }
    }
    else
    {
      // it's ProcDecl
      if ( sym != ident)
      {
        Receiver();
      }
      identdef( procls);
      if ( sym == lparen)
      {
        FormParams();
      }
      accept( semic, 151);
      ProcBody();
      accept( ident, 124);
      InsertId( idbuff, procls);
    }
    accept( semic, 151);
  }

  if ( debugMode) printf( "Out DeclSeq\n");
}

void ConstDecl()
{
  /*
    ConstDecl -> identdef = ConstExpr
  */

  if ( debugMode) printf( "In ConstDecl\n");

  identdef( constcls);
  accept( equal, 123);
  ConstExpr(); 

  if ( debugMode) printf( "Out ConstDecl\n");   
}

void identdef( idclass cls)
{
  /*
    identdef -> ident [ * | - ]
  */

  if ( debugMode) printf( "In identdef\n");
  InsertId( idbuff, cls);
  accept( ident, 124);
  if ( sym == star || sym == hyphen)
  {
    writesym();
    nextsym();
  }

  if ( debugMode) printf( "Out identdef\n");
}

void ConstExpr() {
  /*
    ConstExpr -> expr
  */

  if ( debugMode) printf( "In ConstExpr\n");
  expr();
  if ( debugMode) printf( "Out ConstExpr\n");
}

void TypeDecl()
{
  /*
    TypeDecl -> identdef = StrucType
  */

  if ( debugMode) printf( "In TypeDecl\n");
  // ident should already exist here
  identdef( typcls);
  accept( equal, 123);
  StrucType();
  if ( debugMode) printf( "Out TypeDecl\n");
}

void StrucType()
{
  /*
    StrucType -> ArrayType | RecType | PointerType | ProcType
  */

  if ( debugMode) printf( "In StrucType\n");
  switch( sym) 
  {
    case ARRAY_SYM:
      ArrayType();
      break;
    case RECORD_SYM:
      RecType();
      break;
    case POINTER_SYM:
      PointerType();
      break;
    case PROCEDURE_SYM:
      ProcType();
      break;
    default:
      error( invalid_sym, 1);
  }

  if ( debugMode) printf( "Out StrucType\n");
}

void ArrayType()
{
  /*
    ArrayType -> ARRAY length { , length } OF type
  */

  if ( debugMode) printf( "In ArrayType\n");

  accept( ARRAY_SYM, 157);
  length();

  while ( sym == comma)
  {
    writesym();
    nextsym();
    length();
  }

  accept( OF_SYM, 158);
  type();
  if ( debugMode) printf( "Out ArrayType\n");
}

void length()
{
  /*
    length -> ConstExpr
  */

  if ( debugMode) printf( "In length\n");
  ConstExpr();
  if ( debugMode) printf( "Out length\n");
}

void RecType()
{
  /*
    RecType -> RECORD [ '(' BaseType ')' ] [ FieldListSeq ] END
  */

  if ( debugMode) printf( "In RecType\n");
  accept( RECORD_SYM, 159);

  EnterScop();

  if ( sym == lparen)
  {
    writesym();
    nextsym();
    BaseType();
    accept( rparen, 142);
  }

  if ( sym != END_SYM)
  {
    FieldListSeq();
  }

  ExitScop();
  accept( END_SYM, 155);

  if ( debugMode) printf( "Out RecType\n");
}

void BaseType()
{
  /*
    BaseType -> qualident
  */

  if ( debugMode) printf( "In BaseType\n");
  qualident();
  if ( debugMode) printf( "Out BaseType\n");
}

void qualident()
{
  /*
    qualident -> [ ident . ] ident

    This can be rewritten equivalently as:

    qualident -> ident [ . ident ]
  */

  if ( debugMode) printf( "In qualident\n");
  if ( sym == INTEGER_SYM || sym == REAL_SYM)
  {
    writesym();
    nextsym();
  }
  else
  {
    accept( ident, 124);
  }

  if ( sym == per)
  {
    writesym();
    nextsym();
    accept( ident, 124);
  }
  if ( debugMode) printf( "Out qualident\n");
}

void FieldListSeq()
{
  /*
    FieldListSeq -> FieldList { ; FieldList }
  */

  if ( debugMode) printf( "In FieldListSeq\n");
  FieldList();
  while ( sym == semic)
  {
    writesym();
    nextsym();
    FieldList();
  }
  if ( debugMode) printf( "Out FieldListSeq\n");
}

void FieldList()
{
  /*
    FieldList -> IdentList : type
  */

  if ( debugMode) printf( "In FieldList\n");
  IdentList( varcls);
  accept( colon, 153);
  type();
  if ( debugMode) printf( "Out FieldList\n");
}

void IdentList( idclass cls)
{
  /*
    IdentList -> identdef { , identdef }
  */

  if ( debugMode) printf( "In IdentList\n");
  identdef( cls);
  while( sym == comma)
  {
    writesym();
    nextsym();
    identdef( cls);
  }
  if ( debugMode) printf( "Out IdentList\n");
}

void PointerType()
{
  /*
    PointerType -> POINTER TO type
  */

  if ( debugMode) printf( "In PointerType\n");
  accept( POINTER_SYM, 160);
  accept( TO_SYM, 161);
  type();
  if ( debugMode) printf( "Out PointerType\n");
}

void ProcType()
{
  /*
    ProcType -> PROCEDURE [ FormParams ]
  */

  if ( debugMode) printf( "In ProcType\n");
  accept( PROCEDURE_SYM, 162);
  if ( sym == lparen)
  {
    EnterScop();
    FormParams();
    ExitScop();
  }
  if ( debugMode) printf( "Out ProcType\n");
}

void VarDecl()
{
  /*
    VarDecl -> IdentList : type
  */

  if ( debugMode) printf( "In VarDecl\n");
  IdentList( varcls);
  accept( colon, 153);
  type();
  if ( debugMode) printf( "Out VarDecl\n");
}

void type()
{
  /*
    type -> qualident | StrucType
  */

  if ( debugMode) printf( "In type\n");
  if ( sym == ident || sym == INTEGER_SYM || sym == REAL_SYM)
  {
    qualident();
  }
  else
  {
    StrucType();
  }
  if ( debugMode) printf( "Out type\n");

}


void ProcDecl()
{
  /*
    ProcDecl -> ProcHead ; ProcBody ident
  */

  if ( debugMode) printf( "In ProcDecl\n");
  ProcHead();
  accept( semic, 151);
  ProcBody();
  accept( ident, 124);
  InsertId( idbuff, paramcls);

  ExitScop();

  if ( debugMode) printf( "Out ProcDecl\n");

}

void ProcHead()
{
  /*
    ProcHead -> PROCEDURE [ Receiver ] identdef [ FormParams ]
  */

  if ( debugMode) printf( "In ProcHead\n");
  accept( PROCEDURE_SYM, 162);
  
  if ( sym == lparen)
  {
    // first of receiver
    Receiver();
  }

  identdef( procls);

  EnterScop();
  if ( sym == lparen)
  {
    // first of FormParams
    FormParams();
  }
  if ( debugMode) printf( "Out ProcHead\n");
}

void ProcBody()
{
  /*
    ProcBody -> DeclSeq [ BEGIN StatSeq ] [ RETURN expr ] END
  */

  if ( debugMode) printf( "In ProcBody\n");
  DeclSeq();
  if ( sym == BEGIN_SYM)
  {
    writesym();
    nextsym();
    StatSeq();
  }

  if ( sym == RETURN_SYM)
  {
    writesym();
    nextsym();
    expr();
  }

  accept( END_SYM, 155);
  if ( debugMode) printf( "Out ProcBody\n");
}

void FormParams()
{
  /*
    FormParams -> '(' [ FormParamSect { ; FormParamSect } ] ')' [ : qualident ]
  */

  if ( debugMode) printf( "In FormParams\n");
  accept( lparen, 143);

  if ( sym != rparen) {
    FormParamSect();
    while ( sym == semic)
    {
      writesym();
      nextsym();
      FormParamSect();
    }
  }

  accept( rparen, 142);

  if ( sym == colon)
  {
    writesym();
    nextsym();
    qualident();
  }
  if ( debugMode) printf( "Out FormParams\n");
}

void FormParamSect()
{
  /*
    FormParamSect -> [ VAR ] ident { , ident } : FormType
  */

  if ( debugMode) printf( "In FormParamSect\n");
  if ( sym == VAR_SYM)
  {
    writesym();
    nextsym();
  }

  accept( ident, 124);
  InsertId( idbuff, paramcls);
  while ( sym == comma)
  {
    writesym();
    nextsym();
    accept( ident, 1);
  }

  accept( colon, 153);
  FormType();
  if ( debugMode) printf( "Out FormParamSect\n");
}

void FormType()
{
  /*
    FormType -> { ARRAY OF } ( qualident | ProcType )
  */

  if ( debugMode) printf( "In FormType\n");
  while ( sym == ARRAY_SYM)
  {
    writesym();
    nextsym();
    accept( OF_SYM, 158);
  }

  if ( sym == PROCEDURE_SYM) 
  {
    ProcType();
  }
  else
  {
    qualident();
  }
  if ( debugMode) printf( "Out FormType\n");
}

void ForwardDecl() 
{
  /*
    ForwardDecl -> PROCEDURE ^ [ Receiver ] identdef [ FormParams ]
  */

  if ( debugMode) printf( "In ForwardDecl\n");
  accept( PROCEDURE_SYM, 162);
  accept( carat, 144);
  // first of receiver is (
  if ( sym == lparen)
  {
    Receiver();
  }

  identdef( procls);
  if ( sym == lparen)
  {
    FormParams();
  }
  if ( debugMode) printf( "Out ForwardDecl\n");
}

void Receiver()
{
  /*
    Receiver -> '(' [ VAR ] ident : ident ')'
  */

  if ( debugMode) printf( "In Receiver\n");
  accept( lparen, 143);
  if ( sym == VAR_SYM)
  {
    writesym();
    nextsym();
  }
  accept( ident, 124);
  InsertId( idbuff, varcls);
  accept( colon, 153);
  if ( sym == ident || sym == REAL_SYM || sym == INTEGER_SYM) {
    writesym();
    nextsym();
  }
  else
  {
    error( ident, 124);
  }
  accept( rparen, 142);
  if ( debugMode) printf( "Out Receiver\n");
}

void StatSeq()
{
  /*
    StatSeq -> stat { ; stat }
  */

  if ( debugMode) printf( "In StatSeq\n");
  stat();

  while ( sym == semic)
  {
    writesym();
    nextsym();
    stat();
  }
  if ( debugMode) printf( "Out StatSeq\n");
}

void stat()
{
  /*
    stat -> [ AssignStat | ProcCall | IfStat | CaseStat | WhileStat | 
              RepeatStat | ForStat ]
  */

  if ( debugMode) printf( "In stat\n");
  if ( sym == IF_SYM)
  {
    IfStat();
  }
  else if ( sym == CASE_SYM)
  {
    CaseStat();
  }
  else if ( sym == WHILE_SYM)
  {
    WhileStat();
  }
  else if ( sym == REPEAT_SYM)
  {
    RepeatStat();
  }
  else if ( sym == FOR_SYM)
  {
    ForStat();
  }
  else if ( sym == ident)
  {
    InsertId( idbuff, varcls);  // TODO check if already defined
                                // it should break ostensibly if 
                                // it's a proc call to an undefined proc
    designator();
    if ( sym == assign)
    {
      // then it was AssignStat
      writesym();
      nextsym();
      expr();
    }
    else 
    {
      // then it was ProcCall
      if ( sym == lparen)
      {
        ActParams();
      }
    }
  }
  if ( debugMode) printf( "Out stat\n");
}

void AssignStat()
{
  /*
    AssignStat -> designator := expr
  */

  if ( debugMode) printf( "In AssignStat\n");
  designator();
  accept( assign, 133);
  expr();
  if ( debugMode) printf( "Out AssignStat\n");
}

void ProcCall()
{
  /*
    ProcCall -> designator [ ActParams ]
  */

  if ( debugMode) printf( "In ProcCall\n");
  designator();

  if ( sym == lparen) // first char in ActParams
  {
    ActParams(); // don't call nextsym here since '(' part of ActParams
  }
  if ( debugMode) printf( "Out ProcCall\n");
}

void IfStat()
{
  /*
    IfStat -> IF expr THEN StatSeq
              { ELSIF expr THEN StatSeq }
              [ ELSE StatSeq ] END
  */

  if ( debugMode) printf( "In IfStat\n");
  accept( IF_SYM, 163);
  expr();
  accept( THEN_SYM, 136);
  StatSeq();

  while ( sym == ELSIF_SYM)
  {
    writesym();
    nextsym();
    expr();
    accept( THEN_SYM, 136);
    StatSeq();
  }

  if ( sym == ELSE_SYM)
  {
    writesym();
    nextsym();
    StatSeq();
  }

  accept( END_SYM, 155);
  if ( debugMode) printf( "Out IfStat\n");
}

void CaseStat()
{
  /*
    CaseStat -> CASE expr OF 
                case { '|' case } [ ELSE StatSeq ]END
  */

  if ( debugMode) printf( "In CaseStat\n");
  accept( CASE_SYM, 164);
  expr();
  accept( OF_SYM, 158);
  Case();

  while ( sym == OR_SYM)
  {
    writesym();
    nextsym();
    Case();
  }

  if ( sym == ELSIF_SYM)
  {
    writesym();
    nextsym();
    StatSeq();
  }

  accept( END_SYM, 155);
  if ( debugMode) printf( "Out CaseStat\n");

}

void Case() // uppercase 'Case' since 'case' is reserved in C
{
  /*
    case -> [ CaseLabList : StatSeq ]
  */

  if ( debugMode) printf( "In Case\n");
  // first of CaseLabList is first of LabelRange, which is first of label
  if ( isLabel( sym))  // then, this is ostensibly not a null rule
  {
    CaseLabList();
    accept( colon, 153);
    StatSeq();
  }
  if ( debugMode) printf( "Out Case\n");
}

void CaseLabList()
{
  /*
    CaseLabList -> LabelRange { , LabelRange }
  */

  if ( debugMode) printf( "In CaseLabList\n");
  LabelRange();

  while ( sym == comma)
  {
    writesym();
    nextsym();
    LabelRange();
  }
  if ( debugMode) printf( "Out CaseLabList\n");
}

void LabelRange()
{
  /*
    LabelRange -> label [ .. label ]
  */

  if ( debugMode) printf( "In LabelRange\n");
  if ( isLabel( sym))
  {
    writesym();
    nextsym();
  }
  else 
  {
    error( invalid_sym, 1);
    return;
  }

  if ( sym == doubledot)
  {
    if ( isLabel( sym))
    {
      writesym();
      nextsym();
    }
    else 
    {
      error( invalid_sym, 1);
      return;
    } 
  }
  if ( debugMode) printf( "Out LabelRange\n");
}

void WhileStat()
{
  /*
    WhileStat -> WHILE expr DO StatSeq
                 { ELSIF expr DO StatSeq } END
  */

  if ( debugMode) printf( "In WhileStat\n");
  accept( WHILE_SYM, 165);
  expr();
  accept( DO_SYM, 166);
  StatSeq();

  while ( sym == ELSIF_SYM)
  {
    writesym();
    nextsym();
    expr();
    accept( DO_SYM, 166);
    StatSeq();
  }

  accept( END_SYM, 155);
  if ( debugMode) printf( "Out WhileStat\n");
}

void RepeatStat()
{
  /*
    RepeatStat ->REPEAT StatSeq UNTIL expr
  */

  if ( debugMode) printf( "In RepeatStat\n");
  accept( REPEAT_SYM, 167);
  StatSeq();
  accept( UNTIL_SYM, 168);
  expr();
  if ( debugMode) printf( "Out RepeatStat\n");
}

void ForStat()
{
  /*
    ForStat -> FOR ident := expr TO expr [ BY ConstExpr ] DO StatSeq END
  */

  if ( debugMode) printf( "In ForStat\n");
  accept( FOR_SYM, 169);
  InsertId( idbuff, varcls);
  accept( ident, 124);
  accept( assign, 133);
  expr();
  accept( TO_SYM, 161);

  if ( sym == BY_SYM)
  {
    writesym();
    nextsym();
    ConstExpr();
  }

  accept( DO_SYM, 166);
  StatSeq();
  accept( END_SYM, 155);
  if ( debugMode) printf( "Out ForStat\n");
}

void LoopStat()
{
  /*
    LoopStat -> LOOP StatSeq END
  */

  if ( debugMode) printf( "In LoopStat\n");
  accept( LOOP_SYM, 170);
  StatSeq();
  accept( END_SYM, 155);
  if ( debugMode) printf( "Out LoopStat\n");
}

void ExitStat()
{
  /*
    ExitStat -> EXIT
  */

  if ( debugMode) printf( "In ExitStat\n");
  accept( EXIT_SYM, 171);
  if ( debugMode) printf( "Out ExitStat\n");
}

void WithStat()
{
	/*
		WithStat -> WITH guard DO StatSeq
      				{ | guard DO StatSeq }
      				[ ELSE StatSeq ] END
	*/

  if ( debugMode) printf( "In WithStat\n");
  accept( WITH_SYM, 172);
  guard();
  accept( DO_SYM, 166);
  StatSeq();

  while ( sym == OR_SYM)
  {
    writesym();
    nextsym();
    guard();
    accept( DO_SYM, 166);
    StatSeq();
  } 

  if ( sym == ELSE_SYM) 
  {
    writesym();
    nextsym();
    StatSeq();
  }     			

  accept( END_SYM, 155);	
  if ( debugMode) printf( "Out WithStat\n");
}

void guard() 
{
	/*
		guard -> qualident : qualident
	*/

  if ( debugMode) printf( "In guard\n");
	qualident();
	accept( colon, 153);
	qualident();
  if ( debugMode) printf( "Out guard\n");
}

void expr()
{
	/*
		expr -> SimplExpr [ relop SimplExpr ]
	*/

  if ( debugMode) printf( "In expr\n");
	SimplExpr();

	while ( isRelOp( sym))
	{
    writesym();
		nextsym();
		SimplExpr();
	}
  if ( debugMode) printf( "Out expr\n");
}

void SimplExpr() {
	/* 
		SimplExpr -> [ + | - ] term { addop term }
	*/

  if ( debugMode) printf( "In SimplExpr\n");
	if ( sym == plus || sym == hyphen)
	{
    writesym();
		nextsym();
	}

	term();

	while ( isAddOp( sym))
	{
		nextsym();
		term();
	}
  if ( debugMode) printf( "Out SimplExpr\n");
}

void factor()
{
	/*
		factor -> num | string | NIL | TRUE | FALSE
              | set | designator [ ActParams ] 
              | '(' expr ')'
	*/

  if ( debugMode) printf( "In factor\n");
  if ( isNumber( sym)) {
    writesym();
  	nextsym();
  }
  else {
  	switch( sym)
  	{
  		case string:
        writesym();
  			nextsym();
  			break;
  		case NIL_SYM:
        writesym();
  			nextsym();
  			break;
  		case TRUE_SYM:
        writesym();
  			nextsym();
  			break;
  		case FALSE_SYM:
        writesym();
  			nextsym();
  			break;
      case lparen:
        writesym();
        nextsym();
        expr();
        accept( rparen, 1);
        break;
      case lcurb:
        set();
        break;
  		default:
  			designator();
        if ( sym == lparen)
        {
          ActParams();
        }
  	}
  }
  if ( debugMode) printf( "Out factor\n");
}

void term() 
{
	/*
		term -> factor { mulop factor }
	*/

  if ( debugMode) printf( "In term\n");
	factor();

	while ( isMulOp( sym))
	{
    writesym();
		nextsym();
		factor();
	}
  if ( debugMode) printf( "Out term\n");
}

void set() 
{
	/*
		set -> '{' [ elem { , elem } ] '}'
	*/

  if ( debugMode) printf( "In set\n");
	accept( lcurb, 145);

	if ( sym != rcurb)
	{
		elem();

		while ( sym == comma)
		{
      writesym();
			nextsym();
			elem();
		}
	}

	accept( rcurb, 146);
  if ( debugMode) printf( "Out set\n");
}

void elem()
{
	/*
		elem -> expr [ .. expr ]
	*/

  if ( debugMode) printf( "In elem\n");
	elem();

	if ( sym == doubledot)
	{
    writesym();
		nextsym();
		elem();
	}
  if ( debugMode) printf( "Out elem\n");
}

// TEMPORARY
// void call()
// {
//   /*
//     This is acting in temp of the proc call beginning,
//     to avoid the problem of ( qualident ) in selector vs ( ExprList )
//     in ActParams.
//   */
//   if ( debugMode) printf( "In call\n");
  
//   qualident();
//   if ( sym == per)
//   {
//     // then it's a .wte
//     nextsym();
//     accept( ident, 1);
//   }

//   if ( debugMode) printf( "Out call\n");
// }

void designator()
{
	/*
		designator -> qualident { selector } 
	*/

  if ( debugMode) printf( "In designator\n");
	qualident();

	while ( inSelectorFirstSet( sym))
	{
		// don't call nextsym b/c the sym we're checking 
		// is part of selector (in the first set)
		selector();
	}
  if ( debugMode) printf( "Out designator\n");

}

void selector()
{
	/*
		selector -> . ident 
                | '[' ExprList ']' 
                | ^ 
                | '(' qualident ')'
	*/

  if ( debugMode) printf( "In selector\n");

  switch( sym)
  {
  	case per:
  		nextsym();
  		accept( ident, 1);
  		break;
  	case lbrac:
  		nextsym();
  		ExprList();
  		accept( rbrac, 147);
  		break;
  	case carat:
  		nextsym();
  		break;
  	// case lparen:
  	// 	nextsym();
  	// 	qualident();
  	// 	accept( rparen, 142);
  		break;
  	default:
  		error( invalid_sym, 1);
  		break;
  }
  if ( debugMode) printf( "Out selector\n");
}

void ActParams()
{
	/*
		ActParams -> '(' [ ExprList ] ')'
	*/

  if ( debugMode) printf( "In ActParams\n");
	accept( lparen, 143);
	if ( sym != rparen)
	{
		ExprList();
	}

	accept( rparen, 142);
  if ( debugMode) printf( "Out ActParams\n");
}

void ExprList() {
	/*
		ExprList -> expr { , expr }
	*/

  if ( debugMode) printf( "In expr\n");
	expr();

	while( sym == comma)
	{
    writesym();
		nextsym();
		expr();
	}
  if ( debugMode) printf( "Out expr\n");
}

void ScaleFac()
{
	/*
		ScaleFac -> ( E  | D ) [ + | - ] digit { digit }
	*/

  if ( debugMode) printf( "In ScaleFac\n");
	if ( strcmp( idbuff, "E") == 0 || strcmp( idbuff, "D") == 0)
	{
    writesym();
		nextsym();
	}
	else
	{
		error( invalid_sym, 1);
	}

	if ( sym == plus || sym == hyphen)
	{
    writesym();
		nextsym();
	}

	accept( int_number, 122); // this is digit { digit }
  if ( debugMode) printf( "Out ScaleFac\n");
}













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

   /*do 
   {
     nextsym();
     writesym();
    } while( sym != eof_sym); 
   */

    nextsym();
    module();
    printsymtab();

   return(0);
} // end main
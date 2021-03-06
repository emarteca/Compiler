#include <stdio.h>
#include <stdlib.h> // just using for the exit function
#include "parser.h"


/*
  Oberon scanner + parser + code gen
  Compile:    gcc compiler.c
  Run:        ./a.out fileToCodeGen codeGenOutputFile

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

typedef enum
{
  hlt, ret, neg, add, sub, mulOp, divOp, modOp, 
  eqOp, neqOp, ltOp, lteOp, gtOp, gteOp, orOp, andOp, notl, 
  rdi, rdl, wri, wrl, iabs, isqr, 
  lod, lodc, loda, lodi, sto, stoi, call, isp, 
  jmp, jmpc, for0, for1, nop

  // from Interpret.p
  /*( hlt, ret, neg, add, sub, imul, idiv, imod, 
                eq, ne, lt, le, gt, ge, orl, andl, notl,
                rdi, rdl, wri, wrl, iabs, isqr,  
                lod, lodc, loda, lodi, sto, stoi, isp,
                jmp, jmpc, call, for0, for1, nop) */
} Opcode;

const char *reswrd[ 41][ 50];        // array of reserved words
const char *symname[ 127][ 50];      // array of symbol names
const char *errmsg[ 256][ 32];       // 256 is errmax 
const char *mnemonic[ 38][10];       // instruction mnemonics
Token reswrdsym[ 41];
Token spsym[ 127]; 


FILE* fileIn;       // file reading in from
FILE* fileOut;      // file printing code gen to
char ch;            // current char
Token sym;          // current symbol
char inbuff[ 256];  // inbuffsize
char idbuff[ 16];   // idbuffsize
char qidbuff[ 16];  // idbuffsize (really, this exists for In.Int etc)
char strbuff[ 80];  // max size for string

int intval;         // value of current int being read
int decimals;       // any numbers past the decimal point (if real)
int numdigs;        // number of decimal digits

char hexBuff[ 256]; // let's make the hex numbers have a limit of 256 digs

int inptr;          // pointer to current char in inbuff
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
  constcls, typcls, varcls, proccls, paramcls, stdpcls
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


// code gen 

struct vminstr
{
  Opcode op;  // operation code
  int ld;     // static level difference
  int ad;     // relative displ within stack frame
};

int lc = 0;
const int codemax = 1023;      // max code (upper bound for mem)
struct vminstr code[ 1023];    // code gen'd


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
  copyTo[ i] = '\0';
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

void checktypes( int ttp1, int ttp2)
{
  if ( ttp1 != ttp2)
  {
    printf( "COME ON KILL ME NOW DO IT PLS");
  fputs( inbuff, stdout);
  
    error( TYPE_SYM, 1);
  }
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
  printf( "\nID\tName\tLevel\tType\tPrevid\tAddr");
  printf( "\n");
  int i;
  for( i = 1; i < stptr + 1; ++ i)
  {
    printf( "%d:", i);
    printf( "\t%s", symtab[ i].idname);
    printf( "\t%d", symtab[ i].idlev);
    printf( "\t%d", symtab[ i].idtyp);
    printf( "\t%d", symtab[ i].previd);

    switch( symtab[ i].Class)
    {
      case paramcls:
        printf( "\t%d", symtab[ i].data.pa.paramaddr);
        if ( symtab[ i].data.pa.varparam == 1)
        {
          printf( "\tvarparam");
        }
        else
        {
          printf( "\tvalparam"); 
        }
        break;
      case proccls:
        printf( "\t%d", symtab[ i].data.p.paddr);
        if ( symtab[ i].data.p.resultaddr != 0)
          printf( "\tresultaddr: %d", symtab[ i].data.p.resultaddr);
        break;
      case varcls:
        printf( "\t%d", symtab[ i].data.v.varaddr);
        break;
    }
    printf( "\n");
  }

  printf( "\n");
  for( i = 0; i < currlev + 1; ++ i)
  {
    printf( "\n%d   %d", i, scoptab[ i]);
  }
  printf( "\n");
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
    while( strcmp( symtab[ *stp].idname, id) != 0)// && symtab[ *stp].previd != 0)
    {
      *stp = symtab[ *stp].previd;
      // fputs( symtab[ 0].idname, stdout);
      // printf( "\t");
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
  int stp = 0;
  strcopy( id, symtab[ 0].idname); // sentinel for search
  stp = scoptab[ currlev]; // top of symtab for current scope
  printf( "Here!!!!!!!!!!!!!!!!!!!!!!!!!!%d", stp);
  while( strcmp( symtab[ stp].idname, id) != 0 && stp != 0) 
  {
    //printf( "Here!!!!!!!!!!!!!!!!!!!!!!!!!!%d", stp);
    stp = symtab[ stp].previd; // searching current scope
  }
  if( stp != 0)
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

void InitInstrMnemonics()
{
  mnemonic[    hlt][ 0] = "hlt";
  mnemonic[    ret][ 0] = "ret";
  mnemonic[    neg][ 0] = "neg";
  mnemonic[    add][ 0] = "add";
  mnemonic[    sub][ 0] = "sub";
  mnemonic[  mulOp][ 0] = "imul";
  mnemonic[  divOp][ 0] = "idiv";
  mnemonic[  modOp][ 0] = "imod";
  mnemonic[   eqOp][ 0] = "eq";
  mnemonic[  neqOp][ 0] = "ne";
  mnemonic[   ltOp][ 0] = "lt";
  mnemonic[  lteOp][ 0] = "le";
  mnemonic[   gtOp][ 0] = "gt";
  mnemonic[  gteOp][ 0] = "ge";
  mnemonic[   orOp][ 0] = "or";
  mnemonic[  andOp][ 0] = "and";
  mnemonic[   notl][ 0] = "not";
  mnemonic[   rdi][ 0] = "rdi";
  mnemonic[   rdl][ 0] = "rdl";
  mnemonic[   wri][ 0] = "wri";
  mnemonic[   wrl][ 0] = "wrl";
  mnemonic[  iabs][ 0] = "iabs";
  mnemonic[  isqr][ 0] = "isqr";
  
  mnemonic[   lod][ 0] = "lod";
  mnemonic[  lodc][ 0] = "lodc";
  mnemonic[  loda][ 0] = "loda";
  mnemonic[  lodi][ 0] = "lodi";
  mnemonic[   sto][ 0] = "sto ";
  mnemonic[  stoi][ 0] = "stoi";
  mnemonic[  call][ 0] = "call";
  mnemonic[   isp][ 0] = "isp";
  mnemonic[   jmp][ 0] = "jmp";
  mnemonic[  jmpc][ 0] = "jmpc";
  mnemonic[  for0][ 0] = "for0";
  mnemonic[  for1][ 0] = "for1";
  mnemonic[   nop][ 0] = "nop";
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
  errmsg[  33][ 0] = "Type expected";
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

void gencode( Opcode o, int l, int a)
{
  if( lc > codemax)
  {
    printf( "\nProgram too long - not enough memory!");
    exit( 0);
  }
  code[ lc].op = o;
  code[ lc].ld = l;
  code[ lc].ad = a;
  ++ lc;
}

void listcode(int lc0)
{
  //print to file
  int ilc;
  for( ilc = lc0; ilc < lc; ++ ilc)
  {
    fprintf( fileOut, "%5d %3d %5d\n", code[ ilc].op, code[ ilc].ld, code[ ilc].ad);
  }

}

// call all methods necessary to set up the scanner 
void InitCompile() 
{
  InitResWrds();
  InitResWrdSyms();
  InitSpSyms();
  InitSymNames();
  InitErrMsgs();

  InitSymTab();

  InitInstrMnemonics();
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


// hex number is popred in a buffer
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
  do                // go digit per digit and popre in hex buffer
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

    // first convert the int you have from hex to dec, since we're popring in dec
    // actually i take it back, let's popre hex numbers as 
    // char arrays, then we can print them as hex for output
    int hexInd = rebuffHex( intval); 

    // then, hexInd popres the end of the current int so you can 
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

    int displ = 1;

    if ( debugMode) printf( "In Module\n");

    accept( MODULE_SYM, 154);
    accept( ident, 124);
    //InsertId( idbuff, stdpcls);
    accept( semic, 151);

    EnterScop();

    int savlc = lc;
    gencode( jmp, 0, 0);

    if ( sym == IMPORT_SYM)
    {
    	ImportList();
    }

    DeclSeq( &displ);

    code[ savlc].ad = lc;
    gencode( isp, 0, displ - 1);

    if ( sym == BEGIN_SYM)
    {
      writesym();
    	nextsym();
    	StatSeq();
    }

    ExitScop();

    accept( END_SYM, 155);
    accept( ident, 124);

    gencode( hlt, 0, 0);

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

void DeclSeq( int *displ)
{
  /*
    DeclSeq -> { CONST { ConstDecl ; }
                 | TYPE { TypeDecl ; } 
                 | VAR { VarDecl ; } }
                 { ProcDecl ; }
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
          VarDecl( displ);
          accept( semic, 151);
        }
    }
  }  

  while ( sym == PROCEDURE_SYM)
  {
    ProcDecl();

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
  //InsertId( idbuff, cls);
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

void clearQIDBuff()
{
  int i;
  for ( i = 0; i < idbuffsize; ++ i)
  {
    qidbuff[ i] = '\0';
  }
}

int copyToQID( int indToStart)
{
  int i;
  for ( i = 0; ( i + indToStart) < idbuffsize && idbuff[ i] != '\0'; ++ i)
  {
    qidbuff[ i + indToStart] = idbuff[ i]; 
  }
  return i;
}

void qualident()
{
  /*
    qualident -> [ ident . ] ident

    This can be rewritten equivalently as:

    qualident -> ident [ . ident ]
  */

  if ( debugMode) printf( "In qualident\n");

  clearQIDBuff();

  if ( sym == INTEGER_SYM || sym == REAL_SYM)
  {
    writesym();
    nextsym();
  }
  else
  {
    accept( ident, 124);
  }

  int newInd = copyToQID( 0); // start at beginning

  if ( sym == per)
  {
    qidbuff[ newInd] = '.'; // this should ostensibly deal with In.Int, etc
    ++ newInd;
    writesym();
    nextsym();
    accept( ident, 124);
    copyToQID( newInd);
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
    FormParams( 0, 0); // procptr = 0 for now 
    ExitScop();
  }
  if ( debugMode) printf( "Out ProcType\n");
}

void VarDecl( int *displ)
{
  /*
    VarDecl -> IdentList : type
  */

  if ( debugMode) printf( "In VarDecl\n");
  // IdentList( varcls);

  int ttp, stp1, stp2;

  if ( sym == ident)
  {
    InsertId( idbuff, varcls);
    stp1 = stptr;
    writesym();
    nextsym();
    if ( sym == star || sym == hyphen) 
    {
      writesym();
      nextsym();
    }
  }
  else
  {
    error( ident, 1);
  }

  while ( sym == comma)
  {
    writesym();
    nextsym();
    if ( sym == ident)
    {
      InsertId( idbuff, varcls);
      writesym();
      nextsym();
      if ( sym == star || sym == hyphen)
      {
        writesym();
        nextsym();
      }
    }
    else
    {
      error( ident, 1);
    }
  }
  stp2 = stptr;
  accept( colon, 153);
  type( &ttp);

  do {
    symtab[ stp1].idtyp = ttp;
    symtab[ stp1].data.v.varaddr = *displ;
    *displ = *displ + typtab[ ttp].size;

    ++ stp1;
  } while( stp1 <= stp2);

  if ( debugMode) printf( "Out VarDecl\n");
}

void type( int *ttp)
{
  /*
    type -> qualident | StrucType
  */

  if ( debugMode) printf( "In type\n");
  if ( sym == ident || sym == INTEGER_SYM || sym == REAL_SYM)
  {
    int stp;
    LookupId( idbuff, &stp);
    if ( stp != 0)
    {
      if ( symtab[ stp].Class != typcls)
      {
        printf( "PLPLPLPLPLPLPLPLPL %d ... %d", stp, symtab[ stp].Class);
        error( TYPE_SYM, 33); 
      }
      *ttp = symtab[ stp].idtyp;
      //nextsym();
    }
    qualident(); // ?????
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

  int displ = -2; // displ for param addressing
  int savstptr;

  /*
    ProcHead -> PROCEDURE [ Receiver ] identdef [ FormParams ]
  */

  accept( PROCEDURE_SYM, 162);
  
  if ( sym == lparen)
  {
    // first of receiver
    Receiver();
  }

  // identdef( proccls);
  int procptr;  
  if ( sym == ident)
  {
    InsertId( idbuff, proccls); 
    procptr = stptr; // save ptr to symtab entry for proc
    nextsym();
  }
  else
  {
    error( ident, 1);
  }

  EnterScop();
  savstptr = stptr;
  
  if ( sym == lparen)
  {
    // first of FormParams
    FormParams( procptr, &displ);
  }
  else
  {
    // no param list
    symtab[ procptr].data.p.lastparam = 0;
  }

  accept( semic, 151);
  displ = 1;

  int savlc1 = lc;

  gencode( jmp, 0, 0); // edited later on (placeholder instruction)

  symtab[ procptr].data.p.paddr = lc; // last param
  code[ savlc1].ad = lc;
  gencode( isp, 0, displ - 1);

  /*
    ProcBody -> DeclSeq [ BEGIN StatSeq ] [ RETURN expr ] END
  */

  DeclSeq( &displ);

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
    int ttpRet;
    expr( &ttpRet);
    gencode( sto, 0, symtab[ procptr].data.p.resultaddr);
  }

  gencode( ret, 0, 0); // return

  accept( END_SYM, 155);

  accept( ident, 124);
  //InsertId( idbuff, paramcls);

  ExitScop();
  stptr = savstptr;

  if ( debugMode) printf( "Out ProcDecl\n");

}

void FormParams( int procptr, int *displ)
{
  /*
    FormParams -> '(' [ FormParamSect { ; FormParamSect } ] ')' [ : qualident ]
  */

  if ( debugMode) printf( "In FormParams\n");

  bool isVar;
  int paramptr, stp, paramtyp;

  accept( lparen, 143);

  if ( sym != rparen) {

     /*
      FormParamSect -> [ VAR ] ident { , ident } : FormType
    */
    if ( sym == VAR_SYM)
    {
      isVar = true;
      writesym();
      nextsym();
    }
    else
    {
      isVar = false;
    }

    // accept( ident, 124);
    if ( sym == ident)
    {
      InsertId( idbuff, paramcls);
      paramptr = stptr;
      nextsym();
    }
    else
    {
      error( ident, 124);
    }
    
    
    while ( sym == comma)
    {
      writesym();
      nextsym();
      if ( sym == ident)
      {
        InsertId( idbuff, paramcls);
        nextsym();
      }
      else
      {
        error( ident, 1);
      }
    }

    accept( colon, 153);
    
    /*
    FormType -> { ARRAY OF } ( qualident | ProcType )
    */

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
      LookupId( idbuff, &stp);
      if ( symtab[ stp].Class != typcls)
      {
        error( TYPE_SYM, 1);
      }
      else
      {
        paramtyp = symtab[ stp].idtyp;
      }
      nextsym();
    }

    // end of FormType

    // back-set the types for all the parameters
    while( paramptr <= stptr)
    {
      symtab[ paramptr].idtyp = paramtyp;
      symtab[ paramptr].data.pa.varparam = isVar;
      printf( "!!!!!!!!!!!!!!!!!");
      fputs( symtab[ paramptr].idname, stdout);
      printf( "omfg%d .... \n", symtab[ paramptr].idtyp);
      ++ paramptr;
    }

    // done setting types, back to definition of FormParams

    while ( sym == semic)
    {
      writesym();
      nextsym();

      /*
        FormParamSect -> [ VAR ] ident { , ident } : FormType
      */

      if ( sym == VAR_SYM)
      {
        isVar = true;
        writesym();
        nextsym();
      }
      else
      {
        isVar = false;
      }

      // accept( ident, 124);
      if ( sym == ident)
      {
        InsertId( idbuff, paramcls);
        paramptr = stptr;
        nextsym();
      }
      else
      {
        error( ident, 124);
      }
      
      
      while ( sym == comma)
      {
        writesym();
        nextsym();
        if ( sym == ident)
        {
          InsertId( idbuff, paramcls);
          nextsym();
        }
        else
        {
          error( ident, 1);
        }
      }

      accept( colon, 153);
      
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
        LookupId( idbuff, &stp);
        if ( symtab[ stp].Class != typcls)
        {
          error( TYPE_SYM, 1);
        }
        else
        {
          paramtyp = symtab[ stp].idtyp;
        }
        nextsym();
      }

      // end of FormType

      // back-set the types for all the parameters
      while( paramtyp <= stptr)
      {
        symtab[ paramptr].idtyp = paramtyp;
        symtab[ paramptr].data.pa.varparam = isVar;
        ++ paramptr;
      }


    }
    symtab[ procptr].data.p.lastparam = stptr;

    int lpPtr = symtab[ procptr].data.p.lastparam; // pointer to last param
    int pttptr = 0;

    while ( lpPtr > procptr)
    {
      pttptr = symtab[ lpPtr].idtyp;
      *displ = *displ - typtab[ pttptr].size;
      symtab[ lpPtr].data.pa.paramaddr = *displ;
      -- lpPtr;
    }

  }

  accept( rparen, 142);

  if ( sym == colon)
  {
    writesym();
    nextsym();

    int stp, ttpProc = 0;
    
    if ( sym == ident || sym == INTEGER_SYM || sym == REAL_SYM)
    {
      LookupId( idbuff, &stp);
      if ( symtab[ stp].Class != typcls)
      {
        error( TYPE_SYM, 1);
      }
      else
      {
        symtab[ procptr].idtyp = symtab[ stp].idtyp; // setting return type for proc
      }

    }

    qualident();

    printf( "NOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO%d", ttpProc);
    symtab[ procptr].data.p.resultaddr = *displ - typtab[ symtab[ procptr].idtyp].size; // -1 b/c of static link (???)
  }
  if ( debugMode) printf( "Out FormParams\n");

  printsymtab();
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

void StatSeq( )
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

void stat( )
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
    //InsertId( idbuff, varcls);  // TODO check if already defined
                                // it should break ostensibly if 
                                // it's a proc call to an undefined proc
    int stp = 0, ttp = 0;

    designator();

    LookupId( qidbuff, &stp);
    // fputs( qidbuff, stdout);
    // printf( "PLNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"); 

    // deal with "special" idents (In.Int, etc)

    if ( strcmp( "Out.Int", qidbuff) == 0)
    {
      // print an integer (or list of ints)
      accept( lparen, 1);
      expr( &ttp);
      gencode( wri, 0, 0);

      while( sym == comma)
      {
        nextsym();
        expr( &ttp);
        gencode( wri, 0, 0);
      }
      accept( rparen, 1);
    }
    else if ( strcmp( "In.Int", qidbuff) == 0)
    {
      // reads in one int
      // var must exist already
      accept( lparen, 1);
      LookupId( idbuff, &stp);
      if ( stp != 0)
      {
        if ( symtab[ stp].Class == varcls)
        {
          gencode( loda, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
          gencode( rdi, 0, 0);
        }
        else if ( symtab[ stp].Class == paramcls)
        {
          gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.pa.paramaddr);
          gencode( rdi, 0, 0);
        }
        else
         error( ident, 1);
      }
      else
        error( ident, 1);

      nextsym();
      accept( rparen, 1);
    }
    else if ( strcmp( "In.Int", qidbuff) == 0)
    {
      // TODO
      // for now just skip it
      nextsym();
    }
    else if ( sym == lparen)
    {
      // then it was ProcCall
      int paramlen = 0;
      ActParams( stp, &paramlen); // this gets rid of the lparen

      gencode( call, currlev - symtab[ stp].idlev, symtab[ stp].data.p.paddr);
      gencode( isp, 0, -paramlen);
    } 
    else if ( sym == assign)
    {
      // then it was AssignStat
      printf( "Entering AssignStat");
      fputs( inbuff, stdout);
      writesym();
      nextsym();
      expr( &ttp);

      printf( "WHYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY%d", ttp);
      checktypes( symtab[ stp].idtyp, ttp);
      printf( "OMGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGgg%d%s", symtab[ stp].idtyp, symtab[ stp].idname);
      switch( symtab[ stp].Class)
      {
        case varcls:
          gencode( sto, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
          break;
        case paramcls:
          if( symtab[ stp].data.pa.varparam)
            gencode( stoi, currlev - symtab[ stp].idlev,symtab[ stp].data.pa.paramaddr);
          else
            gencode( sto, currlev - symtab[ stp].idlev,symtab[ stp].data.pa.paramaddr);
          break;
      }
    }
    printf( "Exiting Assignstat");
  }
  if ( debugMode) printf( "Out stat\n");
}

void IfStat()
{
  /*
    IfStat -> IF expr THEN StatSeq
              { ELSIF expr THEN StatSeq }
              [ ELSE StatSeq ] END
  */

  if ( debugMode) printf( "In IfStat\n");

  int savlc1, savlc2[ 256], savlc2Count, ttp;

  accept( IF_SYM, 163);
  expr( &ttp);
  checktypes( ttp, booltyp);
  accept( THEN_SYM, 136);
  savlc1 = lc;
  gencode( jmpc, 0, 0);
  StatSeq();

  savlc2Count = 0;
  savlc2[ savlc2Count] = lc;
  ++ savlc2Count;

  gencode( jmp, 0, 0);
  code[ savlc1].ad = lc;

  while ( sym == ELSIF_SYM)
  {
    writesym();
    nextsym();
    expr( &ttp);
    checktypes( ttp, booltyp);
    accept( THEN_SYM, 136);
    savlc1 = lc;
    gencode( jmpc, 0, 0);
    StatSeq();
    savlc2[ savlc2Count] = lc;
    ++ savlc2Count;
    gencode( jmp, 0, 0);
    code[ savlc1].ad = lc;
  }

  if ( sym == ELSE_SYM)
  {
    writesym();
    nextsym();
    StatSeq();
  }

  int i;
  for( i = 0; i < savlc2Count; ++ i)
  {
    code[ savlc2[ i]].ad = lc;
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

  // case statements here will be implemented like if statements
  // (nested if-else-ifs)

  int ttp, stp, savlc1, savlc2, savlc3[ 256], savlc3Count;

  accept( CASE_SYM, 164);

  LookupId( idbuff, &stp);
  if ( stp == 0)
  {
    error( ident, 1); // should be defined
  }

  expr( &ttp);
  accept( OF_SYM, 158);
  Case( &savlc1, &savlc2, stp);

  savlc3Count = 1;
  savlc3[ savlc3Count] = savlc2;
  ++ savlc3Count;

  while ( sym == OR_SYM)
  {
    writesym();
    nextsym();
    gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
    Case( &savlc1, &savlc2, stp);
    savlc3[ savlc3Count] = savlc2;
    ++ savlc3Count;
  }

  if ( sym == ELSE_SYM)
  {
    writesym();
    nextsym();
    StatSeq();
  }

  int i;
  //printf( "OOOOOOOOOOOOOOOOOOOOOOOMMMMMMMMM%d", savlc3Count);
  for( i = 1; i < savlc3Count; ++ i)     // jump to end of switch from each case
      code[ savlc3[ i]].ad = lc;

  accept( END_SYM, 155);
  if ( debugMode) printf( "Out CaseStat\n");

}

void Case( int *savlc1, int *savlc2, int stp) // uppercase 'Case' since 'case' is reserved in C
{
  /*
    case -> [ CaseLabList : StatSeq ]
  */

  if ( debugMode) printf( "In Case\n");

  int ttp;

  // first of CaseLabList is first of LabelRange, which is first of label
  if ( isLabel( sym))  // then, this is ostensibly not a null rule
  {
    CaseLabList( savlc1, savlc2, stp, &ttp);


    accept( colon, 153);
    StatSeq();

    *savlc2 = lc;
    gencode( jmp, 0, 0);
    code[ *savlc1].ad = lc;
  }
  if ( debugMode) printf( "Out Case\n");
}

void CaseLabList( int *savlc1, int *savlc2, int stp, int *ttp)
{
  /*
    CaseLabList -> LabelRange { , LabelRange }
  */

  if ( debugMode) printf( "In CaseLabList\n");
  LabelRange( savlc1, savlc2, stp, ttp);

  while ( sym == comma)
  {
    writesym();
    nextsym();
    LabelRange( savlc1, savlc2, stp, ttp);
  }
  if ( debugMode) printf( "Out CaseLabList\n");
}

void LabelRange( int *savlc1, int *savlc2, int stp, int *ttp)
{
  /*
    LabelRange -> label [ .. label ]
  */

  if ( debugMode) printf( "In LabelRange\n");
  if ( isLabel( sym))
  {
    writesym();
    if ( isNumber( sym))
      expr( ttp);
    else
      nextsym();
  }
  else 
  {
    error( invalid_sym, 1);
  }

  if ( sym == doubledot)
  {
    nextsym();
    // here, looking at a range x0..x1
    // we want our value to be >= x0, and <= x1
    gencode( gteOp, 0, 0);
    gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
    if ( isLabel( sym))
    {
      writesym();
      if ( isNumber( sym))
      {
        expr( ttp);
        gencode( lteOp, 0, 0);
        gencode( andOp, 0, 0); // AND of the <= x1 and >= x0
        *savlc1 = lc;
        gencode( jmpc, 0, 0);
      }
      else
        nextsym();
    }
  }
  else 
  {
    // not a range
    gencode( eqOp, 0, 0);
    *savlc1 = lc;
    gencode( jmpc, 0, 0);
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

  int savlc1, ttp, savlc2, savlc3[ 256], savlc3Count;

  accept( WHILE_SYM, 165); // WHAWHA

  savlc1 = lc;                // save addr of code for expr
  expr( &ttp); 
  checktypes( ttp, booltyp);  
  //savlc2[ savlc2Count] = lc;
  //++ savlc2Count;
  savlc2 = lc;
  gencode( jmpc, 0, 0);
  accept( DO_SYM, 166);
  StatSeq();
  gencode( jmp, 0, savlc1); // jump back to expr to check
  code[ savlc2].ad = lc;
  
  savlc3Count = 0;
  
  while ( sym == ELSIF_SYM)
  {
    writesym();
    nextsym();
    //savlc1 = lc;
    expr( &ttp);
    //checktypes( ttp, booltyp);
    accept( DO_SYM, 166);
    savlc1 = lc;
    gencode( jmpc, 0, 0);
    //savlc2Count = 0;
    //savlc2[ savlc2Count] = lc;
    //++ savlc2Count;
    //gencode( jmpc, 0, 0);
    StatSeq();
    savlc3[ savlc3Count] = lc;
    ++ savlc3Count;
    gencode( jmp, 0, 0);
    code[ savlc1].ad = lc;
    //gencode( jmp, 0, savlc1);
  }

  int i;
  for( i = 0; i < savlc3Count; ++ i)
  {
    code[ savlc3[ i]].ad = lc;
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

  int savlc, ttp;

  savlc = lc;

  accept( REPEAT_SYM, 167);
  StatSeq();
  accept( UNTIL_SYM, 168);
  expr( &ttp);
  checktypes( ttp, booltyp);
  gencode( jmpc, 0, savlc);
  if ( debugMode) printf( "Out RepeatStat\n");
}

void ForStat()
{
  /*
    ForStat -> FOR ident := expr TO expr [ BY ConstExpr ] DO StatSeq END
  */

  if ( debugMode) printf( "In ForStat\n");

  int ttp1, ttp2;
  int lcvptr, savlc1, savlc2, f;

  accept( FOR_SYM, 169);
  //InsertId( idbuff, varcls);

  accept( ident, 124);
  LookupId( idbuff, &lcvptr); 
  
  accept( assign, 133);
  expr( &ttp1);
  gencode( sto, currlev - symtab[ lcvptr].idlev, symtab[ lcvptr].data.v.varaddr); // pop into update var mem location
  f = 1; // this will store the incrementer - assume += 1 by default
  savlc1 = lc; // this will be where the loop should jump back to after completion
  // push loop incrementer onto stack
  gencode( lod, currlev - symtab[ lcvptr].idlev, symtab[ lcvptr].data.v.varaddr);
  accept( TO_SYM, 161);
  expr( &ttp2);
  checktypes( ttp1, ttp2);

  int checkPtr = lc; // save b/c we'll need up update if it's downto (>=) or to (<=) (depending on if by is -ve)
  gencode( lteOp, 0, 0); // assume it's to unless specified later with the by
  savlc2 = lc;
  gencode( jmpc, 0, 0); // also need to change this later once we know the addr

  bool decr = false;

  if ( sym == BY_SYM)
  {
    writesym();
    nextsym();
    //ConstExpr();
    if ( sym == hyphen)
    {
      // then the by is negative
      // so, change the varcheck from <= to >=
      code[ checkPtr].op = gteOp; // this is opr for ge
      nextsym();
      decr = true;
    }
    nextsym(); // now this should be the number
    f = intval; // this should be read in from the sym
                // also it'll always be +ve since if neg we read in 
                // the minus sign already before reading the int
  }

  accept( DO_SYM, 166);
  // can't use the for code in PLx since this doesn't allow the 
  // possibility of inc/dec by not just 1
  // savlc1 = lc;
  // gencode( for0, f, 0);
  // savlc2 = lc;
  StatSeq();

  gencode( lod, currlev - symtab[ lcvptr].idlev, symtab[ lcvptr].data.v.varaddr);
  gencode( lodc, currlev - symtab[ lcvptr].idlev, f); // incrementer
  if ( decr)
    gencode( sub, 0, 0); // dec
  else
    gencode( add, 0, 0); // inc

  // store new value of loop check var
  gencode( sto, currlev - symtab[ lcvptr].idlev, symtab[ lcvptr].data.v.varaddr);
  gencode( jmp, 0, savlc1); // go back to start of loop

  code[ savlc2].ad = lc; // point to jmp to if loop condition false
  //gencode( for1, f, savlc2);
  //code[ savlc1].ad = lc;
  accept( END_SYM, 155);
  if ( debugMode) printf( "Out ForStat\n");
}

void expr( int *ttp)
{
	/*
		expr -> SimplExpr [ relop SimplExpr ]
	*/

  if ( debugMode) printf( "In expr\n");
  SimplExpr( ttp);

	if ( isRelOp( sym))
	{
    Token relop = sym;
    writesym();
		nextsym();
    int ttp1;
		SimplExpr( &ttp1);

    checktypes( *ttp, ttp1);

    switch( relop)
    {
      case equal:
        gencode( eqOp, 0, 0);
        break;
      case neq:
        gencode( neqOp, 0, 0);
        break;
      case lt:
        gencode( ltOp, 0, 0);
        break;
      case lte:
        gencode( lteOp, 0, 0);
        break;
      case gt:
        gencode( gtOp, 0, 0);
        break;
      case gte:
        gencode( gteOp, 0, 0);
        break;
    }
    *ttp = booltyp; // for expression always bool
	}
  if ( debugMode) printf( "Out expr\n");
}

void SimplExpr( int *ttp) {
	/* 
		SimplExpr -> [ + | - ] term { addop term }
	*/

  if ( debugMode) printf( "In SimplExpr\n");

  Token addop;

	if ( sym == plus || sym == hyphen)
	{
    addop = sym;
    writesym();
		nextsym();
    checktypes( *ttp, inttyp);
    if( addop == hyphen)
      gencode( neg, 0, 0);
	}
	 
  term( ttp);

	while ( isAddOp( sym))
	{
    if ( sym == OR_SYM)
    {
      checktypes( *ttp, booltyp);
    }
    else
    {
      writesym();

      checktypes( *ttp, inttyp);
    }

    addop = sym;
    nextsym();

    int ttp1;
		term( &ttp1);
    checktypes( *ttp, ttp1);

    switch( addop)
    {
      case plus:
        gencode( add, 0, 0);
        break;
      case hyphen:
        gencode( sub, 0, 0);
        break;
      case OR_SYM:
        gencode( orOp, 0, 0);
        break;
    }
	}
  if ( debugMode) printf( "Out SimplExpr\n");
}

void factor( int *ttp)
{
	/*
		factor -> num | string | NIL | TRUE | FALSE
              | set | designator [ ActParams ] 
              | '(' expr ')'
	*/

  if ( debugMode) printf( "In factor\n");

  int stp;

  fputs( inbuff, stdout);
  
  if ( isNumber( sym)) {
    *ttp = inttyp;
    gencode( lodc, 0, intval);

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
        expr( ttp);

        accept( rparen, 1);
        break;
      case lcurb:
        set();
        break;
  		case ident:
        printf( "IIIIIIIIIIIIIIIIIIIIIIIIIIIII");
    
        LookupId( idbuff, &stp);
        if ( stp != 0)
        {
          
          *ttp = symtab[ stp].idtyp;
          printf( "YYYYYYYYYYYYYYYYYYYYYYY%d ......", *ttp);
          fputs( symtab[ stp].idname, stdout);

          switch( symtab[ stp].Class)
          {
            case constcls:
              gencode( lodc, 0, symtab[ stp].data.c.i);
              nextsym();
              break;
            case varcls:

              gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
              nextsym();
              break;
            case paramcls:
              if ( symtab[ stp].data.pa.varparam)
                gencode( lodi, currlev - symtab[ stp].idlev, symtab[ stp].data.pa.paramaddr);
              else
                gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.pa.paramaddr);
              nextsym();
              break;
            case proccls:
              gencode( lodc, 0, 0);

              nextsym();
              int paramlen = 0;
              if ( sym == lparen) // then there are params
              {
                //nextsym();
                ActParams( stp, &paramlen);
              }
              gencode( call, currlev - symtab[ stp].idlev, symtab[ stp].data.p.paddr);
              gencode( isp, 0, -paramlen);
              break; 
            case stdpcls: // standard procedures
              nextsym();
              accept( lparen, 1);
              switch( symtab[ stp].data.sp.procnum)
              {
                case 0:
                  // ABS
                  expr( ttp);
                  LookupId( idbuff, &stp);

                  gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
                  gencode( lodc, 0, 0); // push 0 on stack
                  gencode( ltOp, 0, 0); // check if var < 0
                  int savlc1 = lc;
                  gencode( jmpc, 0, 0);
                  gencode( neg, 0, 0); // negate if value < 0
                  code[ savlc1].ad = lc; // skip negating if negative
                  break;
                case 1: 
                  // ODD
                  expr( ttp);
                  gencode( lodc, 0, 2);
                  gencode( modOp, 0, 0); // div by 2, store output on top of stack
                  break;
            }
            accept( rparen, 1);
            break;
          }
      }
      break;
  	}
  }
  fputs( inbuff, stdout);
  
  if ( debugMode) printf( "Out factor\n");
}

void term( int *ttp) 
{
	/*
		term -> factor { mulop factor }
	*/

  if ( debugMode) printf( "In term\n");

  factor( ttp);

  int ttp1;

	while ( isMulOp( sym))
	{
    if ( sym == AND_SYM)
      checktypes( *ttp, booltyp);
    else
      checktypes( *ttp, inttyp);
    Token mulop = sym;
    writesym();
		nextsym();
		factor( &ttp1);
    checktypes( *ttp, ttp1);
    switch( mulop)
    {
      case star:
        gencode( mulOp, 0, 0);
        break;
      case slash:
        gencode( divOp, 0, 0);
        break;
      case MOD_SYM:
        gencode( modOp, 0, 0);
        break;
      case AND_SYM:
        gencode( andOp, 0, 0);
        break;
    }
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
	int ttp;

  expr( &ttp);

	if ( sym == doubledot)
	{
    writesym();
		nextsym();
    int ttpS;
		expr( &ttpS);
    checktypes( ttp, ttpS);
	}
  if ( debugMode) printf( "Out elem\n");
}

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
  		//ExprList();
      int ttp;
      expr( &ttp);

      while( sym == comma)
      {
        writesym();
        nextsym();
        expr( &ttp);
      }
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

void ActParams( int procptr, int *paramlen)
{
	/*
		ActParams -> '(' [ ExprList ] ')'
	*/

  if ( debugMode) printf( "In ActParams\n");

  int stp, extyp, ttp;

  int nxtparamptr = procptr + 1;

	accept( lparen, 143);
	if ( sym != rparen)
	{
		//ExprList();
    // code from ExprList (so we can call expr with the right params)

    if ( symtab[ nxtparamptr].data.pa.varparam)
    {
      if ( sym == ident)
      {
        LookupId( idbuff, &stp);
        if ( stp != 0)
        {
          checktypes( symtab[ stp].idtyp, symtab[ nxtparamptr].idtyp);
          if ( symtab[ stp].Class == varcls)
          {
            gencode( loda, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
            *paramlen = 1;
          }
          else if ( symtab[ stp].Class == paramcls)
          {
            gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.pa.paramaddr);
          }
          else
          {
            error( invalid_sym, 1);
          }
        } 
        else
          error( ident, 1);
        nextsym();
      }
      else
        error( ident, 1);
    }
    else
    { 
      expr( &ttp);
      *paramlen = 1;
    }

    while( sym == comma)
    {
      writesym();
      nextsym();

      ++ nxtparamptr;

      if ( symtab[ nxtparamptr].data.pa.varparam)
      {
        if ( sym == ident)
        {
          LookupId( idbuff, &stp);
          if ( stp != 0)
          {
            checktypes( symtab[ stp].idtyp, symtab[ nxtparamptr].idtyp);
            if ( symtab[ stp].Class == varcls)
            {
              gencode( loda, currlev - symtab[ stp].idlev, symtab[ stp].data.v.varaddr);
              ++ *paramlen;
            }
            else if ( symtab[ stp].Class == paramcls)
            {
              gencode( lod, currlev - symtab[ stp].idlev, symtab[ stp].data.pa.paramaddr);
            }
            else
            {
              error( invalid_sym, 1);
            }
          } 
          else
            error( ident, 1);
          nextsym();
        }
        else
          error( ident, 1);
      }
      else
      { 
        expr( &ttp);
        ++ *paramlen;
      }
    }
	}

	accept( rparen, 142);
  if ( debugMode) printf( "Out ActParams\n");
}


// run the code
int main( int argc, char **argv) 
{
   ch = ' ';
   InitCompile();

   // arg[1] is the first command line arg

   if ( argc != 3) 
   {
       fputs( "\nError exiting now\nUsage: ./a.out fileToCodeGen codeGenOutputFile\n\n", stdout);
       exit( 0);
   }
   
   fileIn = fopen( argv[ 1], "r");
   fileOut = fopen( argv[ 2], "w");

   /*do 
   {
     nextsym();
     writesym();
    } while( sym != eof_sym); 
   */

    nextsym();
    module();
    printsymtab();

    listcode( 0);
    printf( "\n%d\n", lc);

    fclose( fileIn);
    fclose( fileOut);

   return(0);
} // end main
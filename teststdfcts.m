MODULE teststdfcts;
  IMPORT In, Out;
  VAR a, b: INTEGER;

BEGIN
  
  a := 5;
  b := -2;

  Out.Int( ABS( a));
  Out.Int( ABS( b));

  Out.Int( ODD( a));
  Out.Int( ODD( b));

  Out.Ln;
END teststdfcts.

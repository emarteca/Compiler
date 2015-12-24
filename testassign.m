MODULE testassign;
  IMPORT In, Out;
  VAR a: INTEGER;

BEGIN
  a := 5;
  Out.Int( a);

  a := a + 5 + 5;
  Out.Int( a);

  a := a + 5 - 5 - 5;
  Out.Int( a);

  Out.Ln;
END testassign.

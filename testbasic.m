MODULE testbasic;
  IMPORT In, Out;
  VAR a, b: INTEGER;

BEGIN
  In.Int( a);
  In.Int( b);

  a := a * 2;
  Out.Int( a);

  b := a + b;
  Out.Int( b);

  a := a / 2; (* comment!! also, this value should be original a *)
  Out.Int( a);

  a := a / 2;
  Out.Int( a);

  Out.Ln;
END testbasic.

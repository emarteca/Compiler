MODULE testifelse;
  IMPORT In, Out;
  VAR a: INTEGER;

BEGIN
  a := 5;
  
  IF a = 6 THEN
    Out.Int( 12);
  ELSIF a # 5 THEN
    Out.Int( 13);
  ELSE 
    Out.Int( -1);
  END;

  Out.Ln;
END testifelse.

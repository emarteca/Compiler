MODULE testfor;
  IMPORT In, Out;
  VAR i, tot: INTEGER;

BEGIN
  
  tot := 0;
  
  FOR i := 5 TO 10 DO
    tot := tot + i;
  END;
  Out.Int( tot);

  tot := 0;
  FOR i := 2 TO 11 BY 3 DO
    tot := tot + i;
  END;
  Out.Int( tot);

  tot := 0;
  FOR i := 10 TO 0 BY -1 DO
    tot := tot + i;
  END;
  Out.Int( tot);

  tot := 0;
  FOR i := 10 TO 0 BY -2 DO
    tot := tot + i;
  END;
  Out.Int( tot);

  Out.Ln;
END testfor.

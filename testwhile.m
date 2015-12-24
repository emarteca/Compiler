MODULE testwhile;
  IMPORT In, Out;
  VAR m: INTEGER;

  PROCEDURE runWhile( i: INTEGER); 
  BEGIN
    WHILE ( i > 0) DO
      i := i - 3;
    ELSIF ( i = -2) DO
      Out.Int( 55);
    ELSIF ( i = -1) DO
      Out.Int( 85);
    ELSIF ( i = 0) DO
      Out.Int( 115);
    END;

  END runWhile;

BEGIN
  In.Int( m);
  
  runWhile( m);

  Out.Ln;
END testwhile.

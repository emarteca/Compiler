MODULE tstgcd;
  VAR m: INTEGER;

  PROCEDURE mulByTwo(VAR m: INTEGER): INTEGER;
  BEGIN
    m := m * 2;

  END mulByTwo;

BEGIN
 
  m := 2; 
 
  mulByTwo( m);

  Out.Int( m);
END tstgcd.

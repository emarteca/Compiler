MODULE tstgcd1;
  IMPORT In, Out;
  VAR m, n: INTEGER;

  PROCEDURE gcd(m, n: INTEGER): INTEGER;
  BEGIN
    WHILE m > n DO
      m := m - n
    ELSIF n > m DO
      n := n - m
    END;
    RETURN m
  END gcd;

BEGIN
  In.Int( m);
  In.Int( n);
  
  Out.Int( gcd( m, n), 8);
  Out.Ln;
END tstgcd1.

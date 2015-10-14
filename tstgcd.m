MODULE tstgcd;
  IMPORT In, Out;
  VAR m, n: INTEGER;
  "OMG STRING \"\"\t LOL\""
  "p"

  2..5
  2. 3.5 4.5789 0.345 3. -9

  (*****(**)*****(*fejwofhuwifw*)()()()()(**)lololololol*)

  PROCEDURE gcd(m, n: INTEGER): INTEGER;
  BEGIN
    WHILE m # n DO
      IF m > n THEN
        m := m - n
      ELSE
        n := n - m
      END
    END;
    RETURN m
  END gcd;

BEGIN
  In.Int( m);
  In.Int( n);
  
  Out.Int( gcd( m, n), 8);
  Out.Ln;
END tstgcd.

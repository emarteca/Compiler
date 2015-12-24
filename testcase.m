MODULE testcase;
    VAR a: INTEGER;

BEGIN

  a := 2;

  CASE a OF
    1:  Out.Int( 3);
  | 2:  Out.Int( 2);
  | 3:  Out.Int( 1); 
  END;

  (* testing case with range *)

  a := 7;

  CASE a OF
    1..3:  Out.Int( 13);
  | 4..6:  Out.Int( 12);
  | 7..10:  Out.Int( 11); 
  END;

  CASE a OF
    1..2:  Out.Int( 113);
  | 3..4:  Out.Int( 112);
  | 5..6:  Out.Int( 111);
  ELSE
    Out.Int( 110); 
  END;


  Out.Int( a);
END testcase.

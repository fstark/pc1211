10 REM Comprehensive test program
20 A=42.5 : B$="HELLO"
30 A(100)=3.14159
40 PRINT "Numbers:", A, A(100)
50 PRINT "String:", B$
60 IF A>40 THEN 80
70 PRINT "Should not reach"
80 FOR I=1 TO 3 STEP 1
90 PRINT "Loop:", I
100 NEXT I
110 GOSUB 200
120 PRINT "Back from subroutine"
130 END
200 REM Subroutine
210 PRINT "In subroutine"
220 RETURN

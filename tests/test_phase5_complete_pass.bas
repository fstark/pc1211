10 REM Phase 5 Test: Strings and Type Overwriting
20 A$="HELLO WORLD"
30 PRINT "A$=",A$
40 B=123.45
50 PRINT "B=",B
60 B$="GOODBYE"
70 PRINT "B$=",B$
80 REM Now B should be string type (overwrite test)
90 C$="TOOLONGNAME"
100 PRINT "C$ (truncated)=",C$
110 REM Test indexed assignments
120 A(5)=999
130 PRINT "A(5)=",A(5)
140 A$(7)="INDEXED"
150 PRINT "A$(7)=",A$(7)
160 REM Test type overwriting: A and A$ share storage
170 PRINT "After A(5)=999, A$=",A$
180 A$="NEWVAL"
190 PRINT "After A$=NEWVAL, A(5) is now string type"
200 PRINT "PASS: TEST EXECUTED"

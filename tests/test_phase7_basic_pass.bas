10 REM Phase 7 Test Program
20 REM Test angle modes and device commands
30 PRINT "Testing angle modes..."
40 RADIAN
50 PRINT "RADIAN: COS(", 3.14159/2, ")=", COS(3.14159/2)
60 DEGREE
70 PRINT "DEGREE: COS(90)=", COS(90)
80 GRAD
90 PRINT "GRAD: COS(100)=", COS(100)
100 REM
110 PRINT "Testing CLEAR command..."
120 A = 42
130 B = 99
140 A(5) = 123
150 PRINT "Before CLEAR: A=", A, " B=", B, " A(5)=", A(5)
160 CLEAR
170 PRINT "After CLEAR: A=", A, " B=", B, " A(5)=", A(5)
180 REM
190 PRINT "Testing device commands..."
200 BEEP
210 PAUSE "Waiting 100ms..."
230 USING
240 PRINT "Phase 7 complete!"

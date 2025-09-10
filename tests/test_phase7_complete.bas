10 REM Phase 7 Complete Test
20 REM Test AREAD (must be first!)
30 AREAD A
40 PRINT "AREAD captured:", A
50 REM
60 REM Test angle modes
70 PRINT "Testing angle modes..."
80 RADIAN
90 PRINT "RADIAN: SIN(", 3.14159/2, ")=", SIN(3.14159/2)
100 DEGREE
110 PRINT "DEGREE: SIN(90)=", SIN(90)
120 GRAD
130 PRINT "GRAD: SIN(100)=", SIN(100)
140 REM
150 REM Test inverse trig functions in different modes
160 DEGREE
170 PRINT "DEGREE: ATN(1)=", ATN(1), " (should be 45)"
180 GRAD
190 PRINT "GRAD: ATN(1)=", ATN(1), " (should be 50)"
200 REM
210 REM Test CLEAR command
220 A = 99
230 B = 88
240 A(10) = 77
250 PRINT "Before CLEAR: A=", A, " B=", B, " A(10)=", A(10)
260 CLEAR
270 PRINT "After CLEAR: A=", A, " B=", B, " A(10)=", A(10)
280 REM
290 REM Test device commands
300 PRINT "Testing BEEP..."
310 BEEP
320 PAUSE "Pausing for 100ms...", 123
330 AREAD C
340 PRINT "AREAD after PAUSE:", C, " (should be 0)"
350 USING
360 PRINT "Phase 7 implementation complete!"

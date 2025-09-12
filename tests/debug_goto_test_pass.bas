10 REM Test GOTO/GOSUB with expressions
20 X=100
30 GOTO X
40 PRINT "FAIL: Should not reach here"
50 END
100 PRINT "GOTO with variable worked!"
110 Y=200+10
120 GOSUB Y
130 PRINT "Back from GOSUB"
140 Z=140
150 GOTO Z+100
160 PRINT "FAIL: Should not reach here"
170 END
210 PRINT "GOSUB with expression worked!"
220 RETURN
240 PRINT "PASS: GOTO with complex expression worked!"
260 END

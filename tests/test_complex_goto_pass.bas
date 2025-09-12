10 PRINT "Testing complex GOTO/GOSUB expressions"
20 A=10: B=5: C=200
30 GOTO A*B+50
40 PRINT "FAIL: Should not print this"
100 PRINT "GOTO expression A*B+50 worked!"
110 GOSUB C+A
120 PRINT "PASS: Back from GOSUB expression"
130 END
210 PRINT "GOSUB expression C+A worked!"
220 RETURN

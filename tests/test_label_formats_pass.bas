10 "NOCOLON" PRINT "Label without colon works"
20 "WITH":PRINT "Label with colon also works"
30 GOTO "LBL1"
40 PRINT "FAIL: Should not reach here"
50 END
100 "LBL1" GOTO "LBL2"
110 PRINT "FAIL: Should not reach here"
120 END
200 "LBL2": PRINT "PASS: Reached LBL2"
210 END


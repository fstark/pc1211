10 PRINT "=== PC-1211 GOTO/GOSUB Comprehensive Test ===" 
20 PRINT
30 PRINT "1. Expression GOTO:"
40 A = 100
50 GOTO A + 50
60 PRINT "FAIL: Should not reach here"
70 END
150 PRINT "   Expression GOTO works!"
160 PRINT
170 PRINT "2. String literal GOTO:"
180 GOTO "LABEL1"
190 PRINT "FAIL: Should not reach here"
200 "LABEL1" PRINT "   String literal GOTO works!"
210 PRINT
220 PRINT "3. Computed GOTO (string variable):"
230 B$ = "LABEL2"
240 GOTO B$
250 PRINT "FAIL: Should not reach here"
300 "LABEL2" PRINT "   Computed GOTO works!"
310 PRINT
320 PRINT "4. Expression GOSUB:"
330 X = 400
340 GOSUB X + 50
350 PRINT "   Back from expression GOSUB"
360 PRINT
370 PRINT "5. String literal GOSUB:"
380 GOSUB "SUB1"
390 PRINT "   Back from string literal GOSUB"
400 PRINT
410 PRINT "6. Computed GOSUB (string variable):"
420 C$ = "SUB2"
430 GOSUB C$
440 PRINT "   Back from computed GOSUB"
450 PRINT
460 PRINT "PASS: === ALL TESTS PASSED! ==="
470 END
480 PRINT "   Expression GOSUB works!": RETURN
500 "SUB1" PRINT "   String literal GOSUB works!": RETURN
550 "SUB2" PRINT "   Computed GOSUB works!": RETURN

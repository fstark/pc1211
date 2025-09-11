10 REM Comprehensive label test for PC-1211 BASIC
20 "START":PRINT "Label support working"
30 GOSUB "SUB1"
40 GOSUB 200  
50 "LOOP" B=B+1
60 PRINT "Loop iteration:"; B
70 IF B<3 THEN 50
80 PRINT "Loop finished"
90 GOTO "END"
100 "SUB1":PRINT "Labeled subroutine"
110 RETURN
200 PRINT "Numbered subroutine"
210 RETURN
220 "END" PRINT "All tests passed!"

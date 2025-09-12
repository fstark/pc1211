10 PRINT "Testing mixed labels and line numbers"
20 GOTO "LABEL1"
30 PRINT "Should not print"
40 "LABEL1":PRINT "At label1"
50 GOTO 80
60 PRINT "Should not print"
70 GOTO 90
80 PRINT "At line 80"
90 GOSUB "SUB"
100 PRINT "Back from labeled subroutine"
110 GOSUB 140
120 PRINT "PASS: Back from numbered subroutine"
130 END
140 PRINT "In numbered subroutine"
150 RETURN
160 "SUB":PRINT "In labeled subroutine"
170 RETURN

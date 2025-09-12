10 A$="SUB1"
20 PRINT "Calling subroutine:", A$
30 GOSUB A$
40 PRINT "PASS: Back from subroutine"
50 END
100 "SUB1" PRINT "In subroutine": RETURN

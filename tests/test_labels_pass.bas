10 "START":PRINT "Beginning program"
20 GOTO "MIDDLE"
30 PRINT "Should not print this"
40 "MIDDLE":PRINT "Reached middle"
50 GOSUB "SUB1"
60 PRINT "Back from subroutine"
70 GOTO "END"
80 "SUB1":PRINT "In subroutine 1"
90 RETURN
100 "END":PRINT "Program finished"
110 END

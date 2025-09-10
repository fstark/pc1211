10 PRINT "Testing backward compatibility"
20 GOTO 100
30 PRINT "Should not print this"
100 PRINT "Literal GOTO still works!"
110 GOSUB 200
120 PRINT "Back from literal GOSUB"
130 END
200 PRINT "Literal GOSUB still works!"
210 RETURN

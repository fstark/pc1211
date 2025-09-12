5 C = 100
10 "FOO" PRINT "Target reached!"
20 A$="FOO"
30 PRINT "A$="; A$
35 C = C-1
40 IF C<>0 GOTO A$
50 PRINT "PASS: looped with computed string"

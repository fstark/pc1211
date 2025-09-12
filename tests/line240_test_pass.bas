10 PRINT "Test line 240 issue"
20 Z=140
30 GOTO Z+100
40 PRINT "FAIL: Should not reach line 40"
240 PRINT "PASS: Reached line 240"
250 END

10 REM Test string truncation behavior - this is a very long comment that should not be truncated
20 A$="123456789012345"
30 PRINT "Full string in source:", "123456789012345"
40 PRINT "Variable A$ contains:", A$
50 IF A$="1234567" PRINT "PASS: STRING TRUNCATION":END
60 PRINT "FAIL: STRING TRUNCATION"


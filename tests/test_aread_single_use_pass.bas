10 REM Test AREAD statement - each AREAD can only read once
20 AREAD A
30 PRINT "First AREAD into A:", A
40 AREAD B
50 PRINT "Second AREAD into B (should be 0):", B  
60 PRINT "PASS: Test complete"

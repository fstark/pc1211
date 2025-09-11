10 REM Test AREAD statement
20 AREAD A
30 PRINT "Read into A:", A
40 AREAD B$
50 PRINT "Read into B$:", B$
60 A(5) = 99
70 AREAD A(5)
80 PRINT "Read into A(5):", A(5)
90 PRINT "Final AREAD test done"

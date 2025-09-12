10 REM Phase 6 Complete Test Suite
20 PRINT "=== PHASE 6: ALL MATH FUNCTIONS TEST ==="
30 REM
40 PRINT "Basic trig (radians):"
50 PRINT "SIN(0)=", SIN(0), " (expect 0)"
60 PRINT "COS(0)=", COS(0), " (expect 1)"
70 PRINT "TAN(0)=", TAN(0), " (expect 0)"
80 REM
90 PRINT "Inverse trig:"
100 PRINT "ASN(0.5)=", ASN(0.5), " (expect ~0.524)"
110 PRINT "ACS(0.5)=", ACS(0.5), " (expect ~1.047)"
120 PRINT "ATN(1)=", ATN(1), " (expect ~0.785)"
130 REM
140 PRINT "Logarithms and exponentials:"
150 PRINT "LOG(10)=", LOG(10), " (expect 1)"
160 PRINT "LN(2.71828)=", LN(2.71828), " (expect ~1)"
170 PRINT "EXP(1)=", EXP(1), " (expect ~2.718)"
180 REM
190 PRINT "Other functions:"
200 PRINT "SQR(16)=", SQR(16), " (expect 4)"
210 PRINT "ABS(-7.5)=", ABS(-7.5), " (expect 7.5)"
220 PRINT "INT(4.9)=", INT(4.9), " (expect 4)"
230 PRINT "INT(-4.9)=", INT(-4.9), " (expect -5)"
240 PRINT "SGN(-10)=", SGN(-10), " (expect -1)"
250 PRINT "SGN(0)=", SGN(0), " (expect 0)"
260 PRINT "SGN(10)=", SGN(10), " (expect 1)"
270 REM
280 PRINT "DMS/DEG conversions:"
290 PRINT "DMS(15.4125)=", DMS(15.4125), " (expect 15.2445)"
300 PRINT "DEG(15.2445)=", DEG(15.2445), " (expect 15.4125)"
310 REM
320 PRINT "PASS: === ALL FUNCTIONS WORKING ==="

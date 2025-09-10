10 REM Test various domain errors
20 ON ERROR GOTO 100
30 PRINT "LOG(0)=", LOG(0)
40 PRINT "LOG(-1)=", LOG(-1)  
50 PRINT "ASN(2)=", ASN(2)
60 PRINT "ACS(-2)=", ACS(-2)
70 END
100 PRINT "Domain error caught as expected"
110 END

10 CLS
20 PRINT "FIP BOOT DISK"
25 PRINT "FOR FPGA/NANO"
26 PRINT "PLZ USE BOOT DISK 1"
30 PRINT "1. REAL COCO ON DW"
40 PRINT "2. EMULATED COCO ON DW"
41 PRINT "3. REAL COCO ON SDC"
42 PRINT "4. REAL COCO ON IDE"
43 PRINT "5. EMULATED COCO ON IDE"
44 PRINT "Q. QUIT"
45 PRINT "CHOOSE NO"
50 A$=INKEY$
60 IF A$="" THEN 50
70 IF A$="1" THEN 200
80 IF A$="2" THEN 300
85 IF A$="3" THEN 400
88 IF A$="4" THEN 500
89 IF A$="5" THEN 600
90 IF A$="Q" THEN STOP
110 GOTO 50

200 LOADM"BOOT.BIN":EXEC'DW1 BOOT=REAL.KRN
300 LOADM"BOOT.BIN":EXEC'DW1 BOOT=EMU.KRN
400 LOADM"BOOT.BIN":EXEC'HDB BOOT=REAL.KRN
500 LOADM"BOOT.BIN":EXEC'HDA BOOT=REAL.KRN
600 LOADM"BOOT.BIN":EXEC'HDA BOOT=EMU.KRN

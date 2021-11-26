0 F = 0: te = 0: TS = 0: GOTO 100

1 REM LET T$ = "test desc" : LET S = 0 OR 1 : GOSUB 1
2 IF S THEN PRINT ".";
3 IF NOT S THEN  PRINT " [91mTest [";T$;"] Failed [0m"; : PRINT ""; : F = F + 1
4 TE = TE + 1 : TS = TS + (NOT NOT S) : S = 0 : RETURN

100 PRINT : PRINT "Same line Control";

101 rem "If several statements occur after the THEN, separated by colons, 
102 rem  then they will be executed if and only if the expression is true."

110 T$="Inline If": A$=""   
111 if 0 then A$ = A$ + "true":A$ = A$ + "extra"  
112 S = (A$ = ""):   GOSUB 1  
113 if 1 then A$ = A$ + "true":A$ = A$ + "extra"
114 S = (A$ = "trueextra"): GOSUB 1   

120 T$="Inline For": A$=""  
121 A$ = A$ + "a" : for i=1 to 3: A$ = A$ + "b": next: A$ = A$ + "c"   
122 S = (A$ = "abbbc")  :  GOSUB 1  

130 T$="Inline Gosub": A$=""
131 A$ = A$ + "a" : gosub 132 : A$ = A$ + "c": goto 133
132 A$ = A$ + "b" : return
133 A$ = A$ + "d"
134 S = (A$ = "abcd"): GOSUB 1

140 T$="Inline Goto": A$=""
141 A$ = A$ + "a" : goto 142 : A$ = A$ + "c"
142 A$ = A$ + "b"
143 S = (A$ = "ab"): GOSUB 1

1000 PRINT : PRINT "Variable Control ";

1020 T$ = "Implicit LET"
: A = 123 : S = ( A = 123 ) : GOSUB 1
: A$ = "abc" : S = ( A$ = "abc" ) : GOSUB 1
: A(1) = 234: S = ( A(1) = 234 ) : GOSUB 1
: A$(1) = "bcd" : S = ( A$(1) = "bcd" ) : GOSUB 1

1030 T$ = "Explicit LET"
: LET A = 123 : S = ( A = 123 ) : GOSUB 1
: LET A$ = "abc" : S = ( A$ = "abc" ) : GOSUB 1
: LET A(1) = 234 : S = ( A(1) = 234 ) : GOSUB 1
: LET A$(1) = "bcd" : S = ( A$(1) = "bcd" ) : GOSUB 1

1040 T$ =  "DIM"
: DIM AR(12)
: FOR I = 0 TO 12 : AR(I) = I : NEXT
: T = 0 : U = 0
: FOR I = 0 TO 12 : T = T + I : U = U + AR(I) : NEXT
: S = (T = U) : GOSUB 1

1050 T$ = "DEF FN"
: DEF FN FA(X) = X+X : S = (FN FA(3) = 6) : GOSUB 1
: DEF FN FB(X) = X*X : S = (FN FB(3) = 9) : GOSUB 1

2000 PRINT : PRINT "Flow Control ";

2010 T$ = "GOTO" : T = 1 : GOTO 2017 : T = T + 1
2015 T = T + 1 : GOTO 2019 : T = T + 1
2017 T = T + 1 : GOTO 2015
2018 T = T + 1
2019 S = (T=3) : GOSUB 1

2020 T$ = "GOSUB/RETURN" : T = 1 : GOSUB 2025 : GOTO 2029
2025 T = T + 1 : RETURN
2029 S = (T=2) : GOSUB 1

2030 T$ = "ON GOTO" : T = 1 : ON 2 GOTO 2031, 2032, 2033
2031 T = T + 1
2032 T = T + 1
2033 T = T + 1
2034 S = (T=3) : GOSUB 1

2060 T$ = "ON GOSUB" : T = 1 : ON 2 GOSUB 2061, 2062, 2063 : GOTO 2064
2061 T = T + 1 : RETURN
2062 T = T + 2 : RETURN
2063 T = T + 3 : RETURN
2064 S = (T=3) : GOSUB 1

2160 T$ = "FOR"
: T = 0 : FOR I = 1 TO 10 : T = T + I : NEXT
: S = (T = 55) : GOSUB 1
2161 T$ = "FOR STEP"
: T = 0 : FOR I = 1 TO 10 STEP 2 : T = T + I : NEXT
: S = (T = 25) : GOSUB 1
2162 T$ = "FOR STEP"
: T = 0 : FOR I = 10 TO 1 STEP -1 : T = T + I : NEXT
: S = (T = 55) : GOSUB 1
2163 T$ = "FOR STEP"
: T = 0 : FOR I = 10 TO 1 : T = T + I : NEXT
: S = (T = 10) : GOSUB 1
2164 rem T$ = "FOR Initialization"
: rem I = 20 : FOR I = 6 TO I + 4 : NEXT: print I
: rem S = (I = 11) : GOSUB 1

2170 T$ = "NEXT"
: T = 0 : FOR I = 1 TO 10 : FOR J = 1 TO 10 : FOR K = 1 TO 10 : T = T + 1 : NEXT J, I
: S = (T=100) : GOSUB 1

2171 T$ = "NEXT2"
: T = 0 : FOR I = 1 TO 5 : FOR J = 1 TO 5 : FOR K = 1 TO 5 : T = T + 1 : NEXT K, J, I
: S = (T=125) : GOSUB 1

2172 T$ = "NEXT3"
: T = 0 : FOR I = 1 TO 10 : FOR J = 1 TO 10 : FOR K = 1 TO 10 : T = T + 1 : NEXT I: T = T + 100
: S = (T=110) : GOSUB 1

2180 T$ = "IF THEN"
: T = 1 : IF 0 THEN T = 2
2181 S = (T=1) : GOSUB 1
: T = 1 : IF 1 THEN T = 2 : T = 3
2182 S = (T=3) : GOSUB 1

2190 T$ = "IF GOTO"
: T = 1 : IF 0 GOTO 2192 : T = 2
2191 T = 3
2192 S = (T=3) : GOSUB 1
: T = 1 : IF 1 GOTO 2194 : T = 2
2193 T = 3
2194 S = (T=1) : GOSUB 1

2200 T$ = "Empty String is False"
: T = 1 : IF "" THEN T = 2
2201 S = (T=1) : GOSUB 1

2210 T$ = "Non-Empty String is True"
: T = 1 : IF "abc" THEN T = 2
2211 S = (T=2) : GOSUB 1

5000 PRINT : PRINT "Miscellaneous ";

5010 T$ = "REM" : T = 1 : REM T = 2 : T = 3
5011 S = (T=1) : GOSUB 1

6000 PRINT : PRINT "Inline Data ";

6001 DATA 1,2,3
6002 DATA "A","B","C"
6003 DATA "A","B","C"
6004 DATA "UNTERM,INATED"

6010 T$ = "READ"
: RESTORE
: READ T : S = (T=1) : GOSUB 1
: READ T,U : S = (T=2 AND U=3) : GOSUB 1
: READ A$ : S = (A$="A") : GOSUB 1
: READ A$,B$ : S = (A$="B" AND B$="C") : GOSUB 1
: READ A$,B$,C$ : S = (A$="A" AND B$="B" AND C$="C") : GOSUB 1
: READ A$ : S = (A$="UNTERM,INATED") : GOSUB 1

6020 T$ = "RESTORE" : RESTORE
: READ T : S = (T=1) : GOSUB 1
: READ T,U : S = (T=2 AND U=3) : GOSUB 1
: READ A$ : S = (A$="A") : GOSUB 1
: READ A$,B$ : S = (A$="B" AND B$="C") : GOSUB 1

10000 PRINT : PRINT "Numeric Functions ";
10001 DEF FN E(X) = ABS(X) < 0.001 : REM Within-Epsilon

10010 T$ = "ABS()"
: S = (ABS(1) = 1) : GOSUB 1
: S = (ABS(-1) = 1) : GOSUB 1
: S = (ABS(0) = 0) : GOSUB 1

10050 T$ = "INT()"
: S = (INT(1) = 1) : GOSUB 1
: S = (INT(1.5) = 1) : GOSUB 1
: S = (INT(-1.5) = -2) : GOSUB 1

11000 PRINT : PRINT "String Functions ";

11020 T$ = "LEFT$()"
: S = (LEFT$("ABC",0) = "") : GOSUB 1
: S = (LEFT$("ABC",2) = "AB") : GOSUB 1
: S = (LEFT$("ABC",4) = "ABC") : GOSUB 1

14000 PRINT : PRINT "User Defined Functions ";

14010 T$ = "FN A()"
: DEF FN A(X) = X + X : S = (FN A(3) = 6) : GOSUB 1
: DEF FN A(X) = X * X : S = (FN A(3) = 9) : GOSUB 1

14020 T$ = "FN A$() [language extension]"
: DEF FN A$(X$) = X$ + X$ : S = (FN A$("ABC") = "ABCABC") : GOSUB 1

16000 PRINT : PRINT "Operators ";

16010 T$ = "="
: S = (1 = 1.0) : GOSUB 1
: S = ("ABC" = "ABC") : GOSUB 1

16020 T$ = "<"
: S = (1 < 2) : GOSUB 1

16030 T$ = ">"
: S = (2 > 1) : GOSUB 1

16040 T$ = "<="
: S = (1 <= 1) : GOSUB 1
: S = (1 <= 2) : GOSUB 1
: S = (1 < = 1) : GOSUB 1
: S = (1 < = 2) : GOSUB 1

16050 T$ = ">="
: S = (1 >= 1) : GOSUB 1
: S = (2 >= 1) : GOSUB 1
: S = (1 > = 1) : GOSUB 1
: S = (2 > = 1) : GOSUB 1

16060 T$ = "<>"
: S = (1 <> 2) : GOSUB 1
: S = (1 < > 2) : GOSUB 1
: S = ("A" <> "B") : GOSUB 1
: S = ("A" < > "B") : GOSUB 1

16070 T$ = "AND" : S = (1 AND 1) : GOSUB 1
16080 T$ = "OR" : S = (0 OR 1) : GOSUB 1
16090 T$ = "NOT" : S = (NOT 0) : GOSUB 1

16100 T$ = "^"
: S = (0^0 = 1) : GOSUB 1
: S = (1^1 = 1) : GOSUB 1
: S = (2^2 = 4) : GOSUB 1
: S = (3^0 = 1) : GOSUB 1
: S = (FN E(1.5^-2 - 0.444)) : GOSUB 1

16110 T$ = "*"
: S = (1*0 = 0) : GOSUB 1
: S = (1*1 = 1) : GOSUB 1
: S = (-1*1 = -1) : GOSUB 1
: S = (-1*-1 = 1) : GOSUB 1
: S = (0.5*2 = 1) : GOSUB 1

16120 T$ = "/"
: S = (1/1 = 1) : GOSUB 1
: S = (-1/1 = -1) : GOSUB 1
: S = (-1/-1 = 1) : GOSUB 1
: S = (2/0.5 = 4) : GOSUB 1

16130 T$ = "+"
: S = (0+0 = 0) : GOSUB 1
: S = (0+1 = 1) : GOSUB 1
: S = (-1+1 = 0) : GOSUB 1
: S = (1.5+.5 = 2) : GOSUB 1
: S = ("A"+"B" = "AB") : GOSUB 1
: S = ("A"+"B"+"C" = "ABC") : GOSUB 1
: S = (""+"" = "") : GOSUB 1

16140 T$ = "-"
: S = (0-0 = 0) : GOSUB 1
: S = (1-0 = 1) : GOSUB 1
: S = (-1--1 = 0) : GOSUB 1
: S = (1.5-.5 = 1) : GOSUB 1

16150 T$ = "Precedence"
: S = (FN E((2 + 3 - 4 * 5 / 6 ^ 7) - 4.999)) : GOSUB 1

18000 PRINT : PRINT "Regression Tests ";

18010 T$ = "global rhs in OR"
: T = 0 : FOR I = 1 TO 4
18011 IF I = 1 OR I = 3 GOTO 18014
18012 IF I = 2 OR I = 4 GOTO 18014
18013 T = 1
18014 NEXT
: S = (T=0) : GOSUB 1

18020 T$ = "Operator Associativity"
: S = (10 / 2 * 5 = 25): GOSUB 1
: S = (2 ^ 3 ^ 4 = 4096): GOSUB 1

18050 T$ = "Non-integer Array Indices"
: DIM A2(10) : A2(1) = 123
: S = (A2(1.5) = 123) : GOSUB 1

18090 T$ = "Line Ordering"
18092 T = 2
18091 T = 1
18093 S = (T = 2) : GOSUB 1

20000 PRINT : PRINT : PRINT "Executed tests: "; TE
20010 if F > 0 then PRINT "Successful tests: "; TS : PRINT "[91mFailed tests: "; F;"[0m":STOP

10 text : home
20 print "Prime numbers"
30 print " "; 
40 for x = 1 to 1000
50 gosub 1000
60 if p == 1 then print x;" ";
70 next
80 print
90 end
1000 rem ** subroutine to check for prime **
1010 rem number to be checked is stored in x
1020 rem d is used for divisor
1030 rem q is used for quotient
1040 rem p is used for return value, if x is prime, p will be 1, else 0
1050 p = 0
1060 if x < 2 OR x <> int(x) goto 1180
1070 if x == 2 OR x == 3 OR x == 5 then p=1 : goto 1180
1080 if x/2 == int(x/2) goto 1180
1090 if x/3 == int(x/3) goto 1180
1100 d = 5
1110 q = x/d : if q == int(q) goto 1180
1120 d = d + 2
1130 if d*d> x goto 1170
1140 q = x/d : if q == int(q) goto 1180
1150 d = d + 4
1160 if d*d <= x goto 1110
1170 p = 1
1180 return
1190 rem ** end of subroutine to check for prime **

90 home
95 print "Enter a number to find its factors!"
97 print
100 input a
105 if not a end
110 print
120 let f=0
140 if A/2=int(A/2) then let A=A/2: print "2x";: let f=1: goto 140
145 let i=3
150 let e=int(sqr(a)) + 2
155 let f=0
160 for n=i to e step 2
180 if a/n=int(a/n) then print n;"x";: let a=a/n: let i=n: let n=e: let f=1
200 next n
210 rem print a;" "; n;" "; i;" "; e;" ";f
220 if a>n and f<>0 then goto 155
230 print a
235 print
240 goto 100
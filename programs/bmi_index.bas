10 REM A Simple Body Mass Index Calculator.
20 REM Written by Tim Dwyer on 5 July 2011.
30 HOME
40 DIM height
50 DIM weight
60 DIM bmiCalc
70 DIM keepGoing$
80 PRINT "***************************"
90 PRINT "*                         *"
100 PRINT "*  Simple BMI Calculator  *"
110 PRINT "*                         *"
120 PRINT "***************************"
130 PRINT ""
140 PRINT ""
150 INPUT "Input your height (inches): "; height
160 INPUT "Input your weight (lbs): ";weight
170 bmiCalc = (weight/(height*height))*703
180 PRINT ""
190 PRINT "Your BMI is ";bmiCalc
200 PRINT ""
210 PRINT ""
220 INPUT "Calculate another? (y/n): ";keepGoing$
230 IF keepGoing$ = "y" or keepGoing$ = "Y" GOTO 130
240 IF keepGoing$ = "n" or keepGoing$ = "N" GOTO 300
250 IF keepGoing$ <> "y" or keepGoing$ <> "Y" GOTO 270
260 IF keepGoing$ <> "n" or keepGoing$ <> "N" GOTO 270
270 PRINT ""
280 PRINT "That is not a valid selection." 
290 GOTO 200
300 PRINT ""
310 PRINT "Goodbye!"
320 END

;
;  Designed and written by Daryl Rictor (c)2003-2004
;
;  Modified by Grant Searle 2007
;
;  Define Registers and Port Pins
;
;   Acc = R00, R01 (results of multiply command)
.def  Acc		=	R0			;\ math accumulator
.def  Acch		=	R1			;/ 

.def  curschr	=   R9			; used by terminal to hold the chr under the cursor
.def  temp1s    =   R10
.def  temp0s    =   R11
.def  lastline	=	R12			; 
.def  line1		= 	R13			; 
.def  cmdVal	=	R14			; 
;-------------------------------------------------
.def  J			=	R16			; scratch 8 bit reg
.def  K			=	R17			; scratch 8 bit reg
.def  hpos		=	R18			; hor cursor pos  0-39
.def  vpos		=	R19			; vert cursor pos 0-24
.def  vline		=	R20			;\ vertical display line counter 0-261
.def  vlineh 	=	R21			;/ hi byte of vline counter
.def  chrln		=  	R22			; display chr line 0 - 199
.def  inpt		=	R23			; HOST input 
.def  cursctl	=   R24			; cursor control
.def  cursclk	=	R25			; the cursor blink timer

;    X pair = R26,27
;    Y pair = R28,29
;    Z pair = R30,31
;
;  X pair used to point to SRAM by refresh routines
;  Y pair used to point to SRAM by HOST processor
;  Z pair used to point to Font data in Program Memory & scrolling


;PORT pin defs
.set  syncpin	=	PD1			; output
.set  ldsrpin	=	PD0			; output
.set  ackdpin	=	PA6			; output
.set  ntscpin	=	PA5			; input

;-----------------------------------------

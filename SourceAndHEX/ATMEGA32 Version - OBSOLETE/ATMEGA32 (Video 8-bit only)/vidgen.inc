;******************************************************************************
; NTSC/PAL Video Generation Code (16MHZ System Clock)
;
;  Designed and written by Daryl Rictor (c)2003-2004
;
;  Modified by Grant Searle 2007
;******************************************************************************

	push	acc
	push	accH
	push	ZL
	push	ZH
	push	J
	in		J,SREG
	push 	J

;Get exact sync with clock (remove jitter)
	in      J, TCNT1L				  
	cpi     J, 0x14
	breq	clkSync0
	cpi     J, 0x15
	breq	clkSync1
	cpi     J, 0x16
	breq	clkSync2
	cpi     J, 0x17
	breq	clkSync3
	cpi     J, 0x18
	breq	clkSync4
	nop
	nop
	nop
clkSync0:	 
	nop
	nop
	nop
clkSync1:	 
	nop
	nop
	nop
clkSync2:	 
	nop
	nop
	nop
clkSync3:	 
	nop
	nop
	nop
clkSync4:
	 
; Timer now always 0x24 at this point 
	cbi		PORTD, syncpin			; drop the csync pin (timer = 12)

LINECHECK:
	cpi		vlineh, 0x00		
	brne	NOTFIRST256				

	cpi		vline, 0x04			
	brlo	VSYNC					; no, wait for hsync end
	rjmp	CHECKTOPBLANK

VSYNC:
	rjmp	EOL						; line 0 is a VSYNC line, we're done


CHECKTOPBLANK:
	cp		vline, line1
	brlo	TOPBLANK
	rjmp	CHECKFIRSTLINE

TOPBLANK:
	rjmp	BLANK					 
					

CHECKFIRSTLINE:
	cp		vline, line1
	brne	CHECKLASTLINE

FIRSTLINE:
	ldi		chrln, 0x00				; sync chrlin with first active line	
	rjmp	DISPLAY				

CHECKLASTLINE:
	cpi		chrln, 0xC8				; have we passed line 200?
	brlo	DISPLAY					; no, get ready for active display line
	rjmp	BLANK

NOTFIRST256:
	cpi		chrln, 0xC8				; have we passed line 200?
	brlo	DISPLAY					; no, get ready for active display line

	cp		vline, lastline			
	brne	BLANK					; no, wait for hsync end

	ldi		vlineh, 0x00			; yes, reset vert counters
	ldi		vline, 0x00				;
	cpi		Cursclk, 0xff			; 
	breq	vs1						;
	inc		Cursclk					; inc cursor clock
vs1:


BLANK:
	ldi		chrln, 0xFF				; reset pointer
	rjmp	WAITSYNCEND				; 


DISPLAY:
	; set the SRAM character pointer (X) to the start of the current line (chrln)
	ldi		XL, 0x50				; # chrs per line
	ldi		XH, 0x60				
	mov		J, chrln				; Get current display line
	lsr		J						; divide by 8 
	lsr		J						; 
	lsr		J						; 
	mul		XL, J					; multiply by chrs per line
	ldi		J, 0x60
	add		R0, XH
	adc		R1, J
	movw	XL, R0					; mov it to X reg
	; set Z to point to base of font row
	ldi		ZL, 0x00				; ZL=0, ASCII code will be added for each character
	mov 	ZH, chrln				; ZH= current display line 
	andi	ZH, 0x07				; do ZH MOD 8
	ori		ZH, 0x10				; add $1000 for offset to start of font table

WAITSYNCEND:                        ;  else wait sync pulse end
	in      J, TCNT1L				;  
	cpi     J, 0x50	               	; is it @80 ticks
	brlo    WAITSYNCEND				; no, look again

	in      J, TCNT1L				  
	cpi     J, 0x52
	breq	syncEnd0
	cpi     J, 0x53
	breq	syncEnd1
	cpi     J, 0x54
	breq	syncEnd2
	cpi     J, 0x55
	breq	syncEnd3
	cpi     J, 0x56
	breq	syncEnd4
	cpi     J, 0x57
	breq	syncEnd5
	cpi     J, 0x58
	breq	HSYNCEND
	nop
	nop
	nop
syncEnd0:	 
	nop
	nop
	nop
syncEnd1:	 
	nop
	nop
	nop
syncEnd2:	 
	nop
	nop
	nop
syncEnd3:	 
	nop
	nop
	nop
syncEnd4:	 
	nop
	nop
	nop
syncEnd5:
	nop
	nop
	nop

HSYNCEND:        
	sbi		PORTD, syncpin			; raise sync (73 ticks wide hsync pulse)
	cpi		chrln, 0xFF				; are we on an active display line?
	brne	WAITVISIBLEPORTION		; yes, active display line
	rjmp	EOL						; no, blank line, we're done

WAITVISIBLEPORTION:        
	ldi		J,0
leftBlank:
	inc 	J
	cpi 	J,42
	brlo	leftBlank

	in		J,PORTD		; read current port D
	mov		temp1s,J	; store in temp
	andi	J, 0xFE		; Clear sh/ld bit
	mov		temp0s,J	; store in temp

BUILDSCREEN:
;
; each line consists of 80 macro calls, each 8 clocks long.  Here is the macro:
.macro DispChr
;
;  gets the font byte of the current character and outputs it to the shift register
;	
;  8 clocks

	ld 		ZL, X+				; 2   move curr chr in ZL
	lpm 	J, Z				; 3   get font byte for current chr on curr line
	out		PORTB, J			; 1
	out		PORTD, temp0s		; 1   lower load pin
	out		PORTD, temp1s	    ; 1   raise load pin
.endmacro

	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr


	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr
	DispChr							; display 1 chr

	inc		chrln					; inc line counter

EOL:								; end of line, we're done
	inc		vline					; inc vertical line counter
	brne	eol1
	inc		vlineh					;

eol1:
	pop		J
	out		SREG,J
	pop		J
	pop		ZH
	pop		ZL
	pop		accH
	pop		acc	

	reti						; all done, go back to Main Prgm


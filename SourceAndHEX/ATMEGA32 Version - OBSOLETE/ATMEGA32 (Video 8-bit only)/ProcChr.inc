;******************************************************
; ProcChr - decode host cmd and update disp
;
; Designed and written by Daryl Rictor (c)2003-2004
;
;
;  Modified by Grant Searle 2007
;******************************************************
ProcChr:

	mov		J,cmdVal			; Was previous byte 1st char of a 2-byte command?
	cpi		J, 0x00
	brne	PROCESS_COMMAND_PART_2
	ldi 	ZH, 0x05			; no, setup jump table page address
	mov		ZL, inpt			; put the input chr into Z
	ijmp						; jump table based on INPT


PROCESS_COMMAND_PART_2:
cmd1:
	mov		J,cmdVal			
	cpi		J, 0x0E				; set column
	brne	cmd2
	cpi		inpt, 0x50			; check column = 0-79
	brlo	SET_COL
	rjmp	cmdComplete
SET_COL:
	sub		YL, hpos			;
	sbci	YH, 0x00			; 
	mov		hpos, inpt			; update hpos
	ldi		inpt, 0x00			;
	add		YL, hpos			;
	adc		YH, inpt			;	
	ld		curschr, Y			; save chr under cursor
	rjmp	cmdComplete

cmd2:
	mov		J,cmdVal
	cpi		J, 0x0F				; set row
	brne	cmd3
	cpi		inpt, 0x19			; check row = 0-24
	brlo	SET_ROW
	rjmp	cmdComplete
SET_ROW:
	ldi		YL, 0x50			; # chrs per line
	ldi		YH, 0x60			; offset to start of SRAM 
	andi	inpt, 0x1F			; mask down to Line 0-24
	mov		vpos, inpt			; save new vertical position
	mul		YL, vpos			; multiply by chrs per line
	ldi		inpt, 0x00			; need a zero for upper byte addition
	add		R0, YH				; add SRAM start offset to result
	adc		R1, inpt			; add the carry bit
	movw	YL, R0				; mov it back to Y reg
	add		YL, hpos			; add in hpos
	adc		YH, inpt			; add the carry bit
	ld		curschr, Y			; save chr under cursor
	rjmp	cmdComplete

cmd3:
	mov		J,cmdVal
	cpi		J, 0x1A				; ForceCharIntoRAM
	brne	cmdComplete
	rcall	ASCII

cmdComplete:
	ldi		J,0x00
	mov		cmdVal, J
	ret

TAB:
	ldi		inpt, 0x20			; convert tabs to white spaces
	st		Y+, inpt			; save space & advance
	inc		hpos				; 
	ldi		inpt, 0x07			; tabs are fixed  0,8,16,24,32, 40 (will wrap to next line if=40)	
	and		inpt, hpos			; mask out cursor column
	brne	TAB					; not to next tab, so do again
	rjmp	wrap				; did it wrap?

DEL_EOL:
	ldi		inpt, 0x20			; 
    push	YL
	push	YH
	push	hpos
deleol1:
	st		Y+, inpt			; 
	inc		hpos				; 
	cpi		hpos, 0x50			;
	brne	deleol1				; clear to end of line
	pop		hpos
	pop		YH
	pop		YL
	ld		curschr, Y			; save chr under cursor
	ret

DEL_SOL:
	ldi		inpt, 0x20			; 
    push	YL
	push	YH
	push	hpos
	st		Y,inpt
delsol1:
	st		-Y, inpt			; 
	dec		hpos				; 
	cpi		hpos, 0x00			;
	brne	delsol1				; clear to start of line
	pop		hpos
	pop		YH
	pop		YL
	ld		curschr, Y			; save chr under cursor
	ret

DEL_SOP:						; del to start of page
	ldi		inpt, 0x20			; 
    push	YL
	push	YH
	st		Y,inpt
delsop1:
	st		-Y, inpt			; 
	cpi		YH, 0x00			;
	brne	delsop1				;
	cpi		YL, 0x60
	brne	delsop1
	pop		YH
	pop		YL
	ld		curschr, Y			;
	ret

DEL_EOP:						; del to end of page
	ldi		inpt, 0x20			; 
    push	YL
	push	YH
	st		Y,inpt
deleop1:
	st		Y+, inpt			; 
	cpi		YH, 0x08			;
	brne	deleop1				;
	cpi		YL, 0x30
	brne	deleop1
	pop		YH
	pop		YL
	ld		curschr, Y			;
	ret

UNKNOWN:
	ldi		inpt, 0x3F  ; "?"
	rjmp	ascii

ASCII:
	st		Y+, inpt			; store chr
	inc		hpos				; inc hpos
WRAP:
	cpi		hpos, 0x50			; past eol?
	brne	wrap1				; yes, continue
	ldi		hpos, 0x00			; reset hpos
	inc		vpos				; inc v pos
	cpi		vpos, 0x19			; is it below last line (25)?
	brne	wrap1				; no, do cursor and exit
	sbiw	YL, 0x20			; adjust Y to first char
	sbiw	YL, 0x30			; adjust Y to first char
	ldi		vpos, 0x18			; set to last line (24)
	rjmp	scrollup	
wrap1:
	ld		curschr, Y			; save chr under cursor
	ret							; return to Main

RETURN: 
	sub		YL, hpos			;
	sbci	YH, 0x00			; 
	ldi		hpos, 0x00			; update hpos
	ld		curschr, Y			; save chr under cursor
	
	ret							; return to Main

LINEFEED:
	cpi		vpos, 0x18			; are we at line 24?
	breq	SCROLLUP			; yes, need to scroll up one line
	inc		vpos				; no, move cursor down 1 line
	adiw	YL, 0x20			; adjust Y pointer
	adiw	YL, 0x30			; adjust Y pointer
	ld		curschr, Y			; save chr under cursor
	ret							; return to Main

DELETE:
	ldi		inpt, 0x20			; space
	st		Y, inpt				; save to SRAM
	mov		Curschr, inpt		; save chr under cursor (space)
	ret							; return to Main

BACKSPACE:
	cpi		hpos, 0x00			; are we already at the left edge?
	brne	del1				; no, move left one chr
	cpi		vpos, 0x00			; are we already on the top line?
	breq	del2  				; yes, nothing to do, so end
	dec		vpos				; no, move up one line
	ldi		hpos, 0x50			; & set the cursor to the right-most chr (+1 for next instruction)
del1:	
	dec		hpos				; move cursor left one chr
	ldi		inpt, 0x20			; space
	st		-Y, inpt			; remove last chr (destructive BS)
	mov		Curschr, inpt		; save chr under cursor (space)
del2:
	ret							; return to Main

CURSHOME:						; move cursor to Home w/o clear
	ldi		YH, 0x00			; 
	rjmp	formfeed1

FORMFEED:						; Clear the Screen
	ldi		ZL, 0x60
	ldi		ZH, 0x00			; set to start of SRAM
	ldi		YL, 0xD0
	ldi		YH,	0x07			; set Y to 2000
	ldi		inpt, 0x20			; " " space chracter
ffloop:
	st 		Z+, inpt			; save to SRAM			
	sbiw	YL, 0x01			; dec Y
	brne	ffloop				; do until Y=0
formfeed1:
	ldi		YL, 0x60			; set Y to first chr
	ldi		hpos, 0x00			; set cursor to top left pos
	ldi		vpos, 0x00			; 
	ld		curschr, Y			; save chr under cursor
	ret							; return to Main

SCROLLUP:
	push	YL
	push	YH
	ldi		YL, 0x60			; start of first line
	ldi		YH, 0x00			; 
	ldi		ZL, 0xB0			; start of second line		
	ldi		ZH, 0x00			; 
scup1:
	ld		inpt, Z+			; get line 2 
	st		Y+, inpt			; place in line 1
	cpi		ZL, 0x30
	brne	scup1
	cpi		ZH, 0x08			; at the end of disp RAM?
	brne	scup1				; no, do again
	ldi		inpt, 0x20			; space
scup2:
	st		Y+, inpt			; fill last line with spaces
	cpi		YL, 0x30
	brne	scup2
	pop		YH
	pop		YL
	ld		curschr, Y			; save chr under cursor
	ret

SCROLLDN:
	push	YL
	push	YH
	ldi		YL, 0xE0			; end of line 24
	ldi		YH, 0x07			; 
	ldi		ZL, 0x30			; end of line 25	
	ldi		ZH, 0x08			; 
scdn1:
	ld		inpt, -Y			; get line 2 
	st		-Z, inpt			; place in line 1
	cpi		YL, 0x60
	brne	scdn1
	cpi		YH, 0x00			; at the end of disp RAM?
	brne	scdn1				; no, do again
	ldi		inpt, 0x20			; space
scdn2:
	st		-Z, inpt			; fill last line with spaces
	cpi		ZL, 0x60
	brne	scdn2
	cpi		ZH, 0x00			; at the end of disp RAM?
	brne	scdn2				; no, do again
	pop		YH
	pop		YL
	ld		curschr, Y			; save chr under cursor
	ret

SCROLLLF:
	push	YL
	push	YH
	ldi		YL, 0x60			; start of first line
	ldi		YH, 0x00			; 
	ldi		ZL, 0x61			; start of second line		
	ldi		ZH, 0x00			; 
sclf1:
	ld		inpt, Z+			; get line 2 
	st		Y+, inpt			; place in line 1
	cpi		ZL, 0x30
	brne	sclf1
	cpi		ZH, 0x08			; at the end of disp RAM?
	brne	sclf1				; no, do again
	ldi		inpt, 0x20			; space
sclf2:
	st		Y, inpt				; fill last line with spaces
	sbiw	YL, 0x20				; 
	sbiw	YL, 0x30				; 
	cpi		YL, 0x5F			;
	brne	sclf2				;
	cpi		YH, 0x00			; at the end of disp RAM?
	brne	sclf2				; no, do again
	pop		YH
	pop		YL
	ld		curschr, Y			; save chr under cursor
	ret

SCROLLRT:
	push	YL
	push	YH
	ldi		YL, 0x2F			; end of line 24
	ldi		YH, 0x08			; 
	ldi		ZL, 0x30			; end of line 25	
	ldi		ZH, 0x08			; 
scrt1:
	ld		inpt, -Y			; get line 2 
	st		-Z, inpt			; place in line 1
	cpi		YL, 0x60
	brne	scrt1
	cpi		YH, 0x00			; at the end of disp RAM?
	brne	scrt1				; no, do again
	ldi		inpt, 0x20			; space
scrt2:
	st		Y, inpt				; fill left column with spaces
	adiw	YL, 0x30				; 
	adiw	YL, 0x20				; 
	cpi		YL, 0x30
	brne	scrt2
	cpi		YH, 0x08			; at the end of disp RAM?
	brne	scrt2				; no, do again
	pop		YH
	pop		YL
	ld		curschr, Y			; save chr under cursor
	ret

Curs_us:
	ldi		Cursctl, 0x5F		; no, set cursor to underscore
	ret

Curs_blk:
	ldi		Cursctl, 0xDB		; set cursor to BLOCK
	ret

Curs_off:
	ldi		Cursctl, 0x00
	ret

Curs_blnk:
	ldi		Cursclk, 0x00
	ret

Curs_on:
	ldi		Cursclk, 0xff
	ret

Cursup:
	cpi		vpos, 0x00			; 
	breq	Null				; already at top, nothing to do
	dec		vpos				;
	sbiw	YL, 0x20			; 
	sbiw	YL, 0x30			; 
	ld		curschr, Y			; save chr under cursor
	ret

CursDn:
	cpi		vpos, 0x18			; 
	breq	Null				; already at bottom, nothing to do
	inc		vpos				;
	adiw	YL, 0x20			; 
	adiw	YL, 0x30			; 
	ld		curschr, Y			; save chr under cursor
	ret

Curslf:
	cpi		hpos, 0x00			; 
	breq	Null				; already at top, nothing to do
	dec		hpos				;
	sbiw	YL, 0x01			; 
	ld		curschr, Y			; save chr under cursor
	ret

Cursrt:
	cpi		hpos, 0x4F			; 
	breq	Null				; already at bottom, nothing to do
	inc		hpos				;
	adiw	YL, 0x01			; 
	ld		curschr, Y			; save chr under cursor
	ret

twoByteCommand:
	mov 	cmdVal,inpt			; Store the command ready for the second byte. Process once 2nd byte received
	ret

NULL:
	ret

.cseg
.org 0x500
;
; jump table for each chr input from host
;
	rjmp	null				; 00
	rjmp	CursHome			; 01
	rjmp	Curs_off			; 02 cursor off
	rjmp	Curs_blk			; 03 cursor block
	rjmp	Curs_us				; 04 cursor underscore
	rjmp	Curs_blnk			; 05 cursor blinking
	rjmp	Curs_on				; 06 cursor solid
	rjmp	null				; 07
	rjmp	backspace			; 08
	rjmp	tab					; 09
	rjmp	linefeed			; 0A
	rjmp	null				; 0B
	rjmp	formfeed			; 0C
	rjmp	return				; 0D
	rjmp	twoByteCommand		; 0E SetColumn command (next byte=col number)
	rjmp	twoByteCommand		; 0F SetRow command (next byte=row number)
	rjmp	del_sol				; 10
	rjmp	del_eol				; 11
	rjmp	del_sop				; 12
	rjmp	del_eop				; 13
	rjmp	scrollup			; 14
	rjmp	scrolldn			; 15
	rjmp	scrolllf			; 16
	rjmp	scrollrt			; 17
	rjmp	unknown				; 18
	rjmp	unknown				; 19
	rjmp	twoByteCommand		; 1A Force next byte into RAM (for 00 to 1F chars)
	rjmp	null				; 1B Don't use - reserved for ANSI ESC
	rjmp	CursRt				; 1C
	rjmp	CursLf				; 1D
	rjmp	CursUp				; 1E
	rjmp	CursDn				; 1F
	rjmp	ascii				; 20
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; 30
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; 40
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; 50
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; 60
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; 70
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	delete				; 7F
	rjmp	ascii				; 80
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; 90
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; A0
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; B0
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7 
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B 
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; C0
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; D0
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; E0
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; F
	rjmp	ascii				; F0
	rjmp	ascii				; 1
	rjmp	ascii				; 2
	rjmp	ascii				; 3
	rjmp	ascii				; 4
	rjmp	ascii				; 5
	rjmp	ascii				; 6
	rjmp	ascii				; 7
	rjmp	ascii				; 8
	rjmp	ascii				; 9
	rjmp	ascii				; A
	rjmp	ascii				; B
	rjmp	ascii				; C
	rjmp	ascii				; D
	rjmp	ascii				; E
	rjmp	ascii				; FF

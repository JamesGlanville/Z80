;/******************************************************************************/
;/  NTSC/PAL Video Display (40 character x 25 line monochrome text)					/
;/  8 bit parallel load with latch and busy handshake lines 					/
;/																				/
;/  Designed and written by Daryl Rictor (c)2003-2004							/ 
;/******************************************************************************/

;******************************************************************************
; include ATMega32 definitions & program specific definitions and macros
;******************************************************************************
.include  "m32def.inc"
.include  "defs.inc"
;******************************************************************************
; Code Begins at 0x000
; System IRQ jump table from 0x0000 - 0x0012
;******************************************************************************
	jmp 	RESET 				; Reset Handler
	jmp		EXT_INT0 			; IRQ0 Handler
	jmp 	EXT_INT1 			; IRQ1 Handler
	jmp 	EXT_INT2 			; IRQ1 Handler
	jmp 	TIM2_COMP 			; Timer2 Compare Handler
	jmp 	TIM2_OVF 			; Timer2 Overflow Handler
	jmp 	TIM1_CAPT 			; Timer1 Capture Handler
	jmp 	TIM1_COMPA 			; Timer1 CompareA Handler
	jmp 	TIM1_COMPB 			; Timer1 CompareB Handler
	jmp 	TIM1_OVF 			; Timer1 Overflow Handler
	jmp 	TIM0_COMP 			; Timer1 CompareA Handler
	jmp 	TIM0_OVF 			; Timer0 Overflow Handler
	jmp 	SPI_STC 			; SPI Transfer Complete Handler
	jmp 	USART_RXC 			; USART RX Complete Handler
	jmp 	USART_UDRE 			; UDR Empty Handler
	jmp 	USART_TXC	 		; USART TX Complete Handler
	jmp 	ADCC 				; ADC Conversion Complete Handler
	jmp 	EE_RDY	 			; EEPROM Ready Handler
	jmp 	ANA_COMP 			; Analog Comparator Handler
	jmp 	TWSI 				; Two-wire Serial Interface Handler
	jmp 	SPM_RDY 			; Store Program Memory Ready Handler
	
;******************************************************************************
; unused IRQ vector's point to software reset code
;******************************************************************************

EXT_INT0:
EXT_INT1:
EXT_INT2:
TIM2_COMP:
TIM2_OVF:
TIM1_CAPT:
;TIM1_COMPA:
TIM1_COMPB:
TIM1_OVF:
TIM0_COMP:
TIM0_OVF:
SPI_STC:
USART_RXC:
USART_UDRE:
USART_TXC:
ADCC:
EE_RDY:
ANA_COMP:
TWSI:
SPM_RDY:

;******************************************************************************
; Program Starts here on Reset
;******************************************************************************
RESET:							; first line executed after reset 
	CLI							; disable interupts

	ldi		J,0x00
	out		UCSRB,J
	mov		cmdVal, J

;******************************************************************************
; Initialize the I/O Ports
;******************************************************************************
; port B pins (video data bus)
	ldi		J, 0x00				; data 0
	out		PORTB, J			; set port B outputs to 0
	LDI		J, 0xFF				; pins 0 - 7 of port B
	out		DDRB, J				; set PORTB pins to outputs

; port C pins (host data input bus)
	LDI		J, 0x00				; all 8 pins of port D
	out		DDRC, J				; set PORTD pins 0-7 to inputs
	out		PORTC, J			; disable pullups on port C

; port A and D pins (control, handshake)
	LDI		J, 0xFF				; all 8 pins of port D
	out		PORTA, J			; enable pullups on portA
	SBI		PORTD, syncpin		; turn on sync bit (done after mode select below)
	SBI		PORTD, ldsrpin		; turn shift load bit on 
	CBI		PORTA, ackdpin		; set ack pin to low
	LDI		J, 0x40				; 
	out		DDRA, J				; PA6 is output
	LDI		J, 0x03				; PC0 and PC1 is output
	out		DDRD, J			

; Load Shift Register with all 0' so display remains blank
	CBI		PORTD, ldsrpin		; set SR to LOAD mode
	SBI		PORTD, ldsrpin		; set SR to SHIFT mode

;******************************************************************************
; Select PAL/NTSC System
;******************************************************************************

	sbis	PINA, ntscpin		; is Pin A5 low?
	rjmp	SETNTSC				; Yes, set NTSC mode

SETPAL:							; no, set PAL mode
	ldi		J, 0x38				; 0x0138 Last vertical line 312
	mov		lastline, J			;
	ldi		J, 0x40				; first vertical line of active disp
	mov		line1, J			;
	ldi		J, 0x04				;  0x0400 = 1024 clocks per line		
	out     OCR1AH, J        
	ldi		J, 0x00				;  0x0400 = 1024 clocks per line		
	out     OCR1AL, J			; set Counter 1 to cycle from 0 to 1016 (1024 PAL) and IRQ
	rjmp	PAL_NTSC_DONE		;

SETNTSC:						
	ldi		J, 0x06				; 0x0106 Last vertical line 262
	mov		lastline, J			;
	ldi		J, 0x28				; first vertical line of active disp
	mov		line1, J			;
	ldi		J, 0x03				; 0x03f8 = 1016 clocks per line (63.5us)
	out     OCR1AH, J        
	ldi		J, 0xf8				; 0x03f8 = 1016 clocks per line (63.5us)
	out     OCR1AL, J			; set Counter 1 to cycle from 0 to 1016 (1024 PAL) and IRQ
	
PAL_NTSC_DONE:

;******************************************************************************
; Initialize Timer 1 
;******************************************************************************
;   OCR1A will hold the value of 1016/1024 and be used to reset the counter and 
;        cause an IRQ that will start the HSYNC process
;   The counter will count each clock cycle (16MHz - no prescaling)

	ldi     J, 0x00
	out     TCCR1A, J
	ldi     J, 0x09				; set timer 1 to CTC mode 4
	out     TCCR1B, J		

	ldi     J, 0x10				; allow OCR1A IRQ's
	out     TIMSK, J			

;******************************************************************************
; Init other CPU I/O registers
;******************************************************************************
	ldi     J, 0x80
	out     ACSR, J				; turn off comparator

	ldi     J, 0x80
	out		MCUCR, J			; enable sleep mode

	ldi		J, 0x08				; set stack pointer to top of SRAM 
	out		SPH, J				; (24 bytes max stack size)
	ldi		J, 0x5f				; uses 6 bytes (IRQA-2, IRQB-2, ProcChr sub-2
	out		SPL, J				;

;******************************************************************************
; Initialize Program Variables
;******************************************************************************
	ldi		chrln, 0x00			; keeps track of font gen line (0-7)
	ldi		vlineh, 0x00		; vertical line counter 0-262
	ldi		vline, 0x00			;  ""
	ldi		YL, 0x60			;
	ldi		YH, 0x00			; init y pointer
	ldi		hpos, 0x00			; keeps track of horizontal chr pos (0-39)
	ldi		vpos, 0x00			; keeps track of vertical chr pos (0-24)
	ldi		Cursclk, 0x00		; init cursor blink timer
	ld		Curschr, Y			; init cursor character
	ldi		Cursctl, 0x5F		; init cursor '_' character
	ldi		J,	0x00			; 0

;******************************************************************************
;  Reset Timer 1 and clear Busy Status Flag
;******************************************************************************
	ldi     J, 0x00				; clear timer 1
	out     TCNT1H, J			; 
	out     TCNT1L, J			; 

	ldi		J, 0x1C				; clear timer 1 IRQ's
	out		TIFR, J				; clear timer 1 IRQ's

	CBI		PORTA, ackdpin		; clear the ACK line to the SBC 
	SBI		PORTA, ackdpin		; 

;	ldi		CursClk, 0xFF		 

	ldi 	inpt,0x0C			; CLS
	rcall	ProcChr
	ldi 	inpt,0xBE			; Cursor blink
	rcall	ProcChr
							 
	SEI							; Ready to go - enable system IRQ's
		
;******************************************************************************
; Main Loop 
;******************************************************************************
Main:
	cpi		Cursctl, 0x00		; is the cursor turned on?
	breq	main2				; no, skip control code
	cpi		Cursclk, 0xff		; is clk at 0xFF?
	breq	main1				; yes - solid cursor (no blink)
	cpi		Cursclk, 0x1C		; is it above 28 frames?
	brpl	main2				; if >25, show Cursor
	st		Y, Curschr			; if clr, show character
	rjmp	main3				; 
main1:
	st		Y, Cursctl			; if set show cursor
	rjmp	main3
main2:
	st		Y, Cursctl			; if set show cursor
	cpi		Cursclk, 0x37		; is it >56 
	brmi	main3				; no
	clr		Cursclk				; reset timer
main3:

	; If the ack and data rdy are the same state then there is no
	; data waiting
	in		J,PINA
	andi	J,0x80
	in		K,PORTA
	andi	K,0x40
	lsl		K
	eor		J,K
	breq	Main

	st		Y, Curschr			; restore chr under cursor
	in		inpt, PINC			; save data

	; flip the ack bit
	in		K,PORTA
	ldi		J,0x40
	eor		K,J
	out		PORTA,K

	rcall	ProcChr				; process the host data
	rjmp	Main				; repeat

;******************************************************************************
; IRQ Service Routines
;******************************************************************************

TIM1_COMPA:						; IRQ service for OCR1A 
	.include "vidgen.inc"		; file that holds the video generation code	
	reti						; done

;******************************************************************************
; include subroutines and Constant Data tables
;******************************************************************************
.include "ProcChr.inc"			; terminal emulation subroutines 0x500-0x5FF
;.include "banner.inc"			; opening screen banner 0x600-0x7F3
.include "CGABoldfont.inc"			; display font 0x800-0xBFF
;.include "primfont.inc"			; disp PRIMARY font 0x800-0xBFF
;.include "altfont.inc"			; disp ALT font 0xC00-0xFFF

;******************************************************************************
; End of Program
;******************************************************************************


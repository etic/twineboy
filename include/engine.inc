;****************************************************************************************************************************************************
;*	Twineboy.z80 - Twine engine for Nintendo Game Boy
;*  Created by Chris Lewis (Skeleton from Jeff Frohwein)
;*
;****************************************************************************************************************************************************
;*
;*
;****************************************************************************************************************************************************
	
;****************************************************************************************************************************************************
;*	user data (constants)
;****************************************************************************************************************************************************
	SECTION "Low Ram",BSS

StateFlags		DB 
; 00000001 - blink on
; 00000010 - line change
; 00000100 - choice change
; 00001000 - passage change
TempPosition	DW
TempCurrentLine	DW
TempChoiceLine	DW
TempChoiceColumn	DB
TempChoiceLength	DB
TempChoicePassage 	DW
CursorCounter 	DB
CurrentLine 	DW
TotalLines 		DW
CurrentChoice 	DB
TotalChoices 	DB
ChoiceStack 	DS 	252
LineStack 		DS 	2048
CustomLineStack	DS 	640
ChoiceBuffer	DS 	256

;****************************************************************************************************************************************************
;*	equates
;****************************************************************************************************************************************************

CURSOR_BLINK	EQU 	20
CURSOR_BLINK_MAX EQU 	40
CURSOR_CHAR 	EQU 	47


;***************************************************************
;* Macros
;***************************************************************

; load from address in 16-bit register to other 16 bit register
ld16ra:	MACRO
__r1    EQUS    STRSUB("\1",1,1)
__r2    EQUS    STRSUB("\1",2,1)
		ld __r1,[\2]
		inc \2
		ld __r2,[\2]
		dec \2
PURGE 	__r1
PURGE 	__r2
		ENDM

; load from 16-bit register to address in other 16 bit register
ld16ar:	MACRO
__r1    EQUS    STRSUB("\2",1,1)
__r2    EQUS    STRSUB("\2",2,1)
		ld [\1],__r1
		inc \1
		ld [\1],__r2
		dec \1
PURGE 	__r1
PURGE 	__r2
		ENDM

; load from address to 16 bit register (not HL)
ld16r:	MACRO	
__r1    EQUS    STRSUB("\1",1,1)
__r2    EQUS    STRSUB("\1",2,1)	
		push hl
			ld hl,\2
			ld __r1,[hl]
			inc hl
			ld __r2,[hl]
		pop hl
PURGE 	__r1
PURGE 	__r2
		ENDM

; load from 16-bit register to address (not HL)
ld16a:	MACRO
__r1    EQUS    STRSUB("\2",1,1)
__r2    EQUS    STRSUB("\2",2,1)
		push hl
			ld hl,\1
			ld [hl],__r1
			inc hl
			ld [hl],__r2
		pop hl
PURGE 	__r1
PURGE 	__r2
		ENDM

; load from 16-bit register to 16-bit register
ld16rr:	MACRO
__r1a	EQUS 	STRSUB("\1",1,1)
__r1b 	EQUS 	STRSUB("\1",2,1)
__r2a	EQUS 	STRSUB("\2",1,1)
__r2b 	EQUS 	STRSUB("\2",2,1)
		ld __r1a,__r2a
		ld __r1b,__r2b
PURGE 	__r1a
PURGE 	__r1b
PURGE 	__r2a
PURGE 	__r2b
		ENDM

; 16-bit complement
cpl16:	MACRO
__r1    EQUS    STRSUB("\1",1,1)
__r2    EQUS    STRSUB("\1",2,1)
		push af
			ld a,__r1
			cpl
			ld __r1,a
			ld a,__r2
			cpl
			ld __r2,a
		pop af
PURGE 	__r1
PURGE 	__r2
		ENDM

;****************************************************************************************************************************************************
;*	cartridge header
;****************************************************************************************************************************************************

	SECTION	"Org $00",HOME[$00]
RST_00:	
	jp	$100

	SECTION	"Org $08",HOME[$08]
RST_08:	
	jp	$100

	SECTION	"Org $10",HOME[$10]
RST_10:
	jp	$100

	SECTION	"Org $18",HOME[$18]
RST_18:
	jp	$100

	SECTION	"Org $20",HOME[$20]
RST_20:
	jp	$100

	SECTION	"Org $28",HOME[$28]
RST_28:
	jp	$100

	SECTION	"Org $30",HOME[$30]
RST_30:
	jp	$100

	SECTION	"Org $38",HOME[$38]
RST_38:
	jp	$100

	SECTION	"V-Blank IRQ Vector",HOME[$40]
VBL_VECT:
	reti
	
	SECTION	"LCD IRQ Vector",HOME[$48]
LCD_VECT:
	reti

	SECTION	"Timer IRQ Vector",HOME[$50]
TIMER_VECT:
	reti

	SECTION	"Serial IRQ Vector",HOME[$58]
SERIAL_VECT:
	reti

	SECTION	"Joypad IRQ Vector",HOME[$60]
JOYPAD_VECT:
	reti
	
	SECTION	"Start",HOME[$100]
	nop
	jp	Start

	; $0104-$0133 (Nintendo logo - do _not_ modify the logo data here or the GB will not run the program)
	DB	$CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	DB	$00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	DB	$BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

	; $0134-$013E (Game title - up to 11 upper case ASCII characters; pad with $00)
	DB	$00,$00,"TWINEBOY"
		;0123456789A

	; $013F-$0142 (Product code - 4 ASCII characters, assigned by Nintendo, just leave blank)
	DB	"    "
		;0123

	; $0143 (Color GameBoy compatibility code)
	DB	$00	; $00 - DMG 
			; $80 - DMG/GBC
			; $C0 - GBC Only cartridge

	; $0144 (High-nibble of license code - normally $00 if $014B != $33)
	DB	$00

	; $0145 (Low-nibble of license code - normally $00 if $014B != $33)
	DB	$00

	; $0146 (GameBoy/Super GameBoy indicator)
	DB	$00	; $00 - GameBoy

	; $0147 (Cartridge type - all Color GameBoy cartridges are at least $19)
	DB	$00	; $00 - ROM Only

	; $0148 (ROM size)
	DB	$00	; $00 - 256Kbit = 32Kbyte = 2 banks

	; $0149 (RAM size)
	DB	$00	; $00 - None

	; $014A (Destination code)
	DB	$00	; $01 - All others
			; $00 - Japan

	; $014B (Licensee code - this _must_ be $33)
	DB	$33	; $33 - Check $0144/$0145 for Licensee code.

	; $014C (Mask ROM version - handled by RGBFIX)
	DB	$00

	; $014D (Complement check - handled by RGBFIX)
	DB	$00

	; $014E-$014F (Cartridge checksum - handled by RGBFIX)
	DW	$00


;****************************************************************************************************************************************************
;*	Program Start
;****************************************************************************************************************************************************

	SECTION "Program Start",HOME[$0150]
Start::
	di			;disable interrupts
	ld	sp,$FFFE	;set the stack to $FFFE
	call WAIT_VBLANK	;wait for v-blank

	ld	a,0		;
	ldh	[rLCDC],a	;turn off LCD 
	
	ld	a,%00000011
	ld	[rIE],a

	;call CLEAR_MAP	;clear the BG map
	call CLEAR_MAP
	call CLEAR_OAM
	call CLEAR_VARS
	call LOAD_TILES	;load up our tiles
	call CLEAR_PASSAGE_RAM

	ld a,[Passage_INIT]
	ld h,a
	ld a,[Passage_INIT+1]
	ld l,a
	ld a,0
	call LOAD_PASSAGE
	call LOAD_CHOICE_BUFFER
	;call LOAD_MAP	;load up our map
	;call LOAD_SOUND ;make sound channel 1 have noise
	;call LOAD_SPRITE	;have a movey walky thing

	ld	a,%11100100	;load a normal palette up 11 10 01 00 - dark->light
	ldh	[rBGP],a	;load the palette
	ldh [rOBP0],a	;load the palette into OBJ0PAL, final 2 bits are the "transparency" color (which doesn't show up)
	ld	a,%10010011		;  =$91 
	ldh	[rLCDC],a	;turn on the LCD, BG, etc

	di
	call UPDATE_SCREEN_LINES
	ei

Loop::
	halt
	di
	nop
	call pad_Read
	call UPDATE_INPUT
	call HANDLE_EVENTS
	call UPDATE_CURSOR_BLINK
	;call UPDATE_CURSOR
	;call MOVE_SPRITE
	ei
	jp Loop

;***************************************************************
;* Subroutines
;***************************************************************

	SECTION "Support Routines",HOME

WAIT_VBLANK::
	ldh	a,[rLY]		;get current scanline
	cp	$91			;Are we in v-blank yet?
	jr	nz,WAIT_VBLANK	;if A-91 != 0 then loop
	ret				;done
	
CLEAR_MAP::
	ld	hl,_SCRN0		;loads the address of the bg map ($9800) into HL
	ld	bc,32*32		;since we have 32x32 tiles, we'll need a counter so we can clear all of them
CLEAR_MAP_LOOP::
	ld	a,0			;load 0 into A (since our tile 0 is blank)
	ld	[hl+],a		;load A into HL, then increment HL (the HL+)
	dec	bc			;decrement our counter
	ld	a,b			;load B into A
	or	c			;if B or C != 0
	jr	nz,CLEAR_MAP_LOOP	;then loop
	ret				;done
	
CLEAR_OAM::
	ld	hl,_OAMRAM
	ld	bc,4*40
CLEAR_OAM_LOOP:
	ld	a,0
	ld	[hl+],a		;load A into HL, then increment HL (the HL+)
	dec	bc			;decrement our counter
	ld	a,b			;load B into A
	or	c			;if B or C != 0
	jr	nz,CLEAR_OAM_LOOP	;then loop
	ret				;done
	
CLEAR_VARS::
	ld a,0
	ld [CursorCounter],a
	ld [StateFlags],a
	ret

LOAD_TILES::
	ld	hl,ASCII_TILES
	ld	de,_VRAM
	ld	bc,128*16	;we have 128 tiles and each tile takes 16 bytes
LOAD_TILES_LOOP::
	ld	a,[hl+]	;get a byte from our tiles, and increment.
	ld	[de],a	;put that byte in VRAM and
	inc	de		;increment.
	dec	bc		;bc=bc-1.
	ld	a,b		;if b or c != 0,
	or	c		;
	jr	nz,LOAD_TILES_LOOP	;then loop.
	ret			;done


HANDLE_EVENTS::
	ld a,[StateFlags]
	bit 1,a ; line change
	jp z,hanev1
	push af
		call UPDATE_SCREEN_LINES
	pop af
hanev1::
	bit 2,a ; choice change
	jp z,hanev2
	push af
		call UPDATE_SCREEN_CHOICE
	pop af
hanev2::
	bit 3,a ; passage change
	jp z,hanev3
	push af
		call UPDATE_SCREEN_PASSAGE
	pop af
hanev3::
	ret

UPDATE_SCREEN_CHOICE::
	call GET_NEW_CHOICE_LINE
	call MOVE_LINE_ONSCREEN
	call GET_CHOICE_START_VRAM
	call UPDATE_CURSOR_BLINK_OFF
	call LOAD_CHOICE_BUFFER
	ret

UPDATE_INPUT::
	; up and down (changes current line)
	ld a,[StateFlags]
	and %11111101
	ld [StateFlags],a
	ld a,[_PadDataEdge]
	bit 6,a
	jp z,ucl_pressDown
	ld hl,CurrentLine
	call LD_HLA_TO_BC
	dec bc
	call LD_BC_TO_HLA
	ld a,[StateFlags]
	or %00000010
	ld [StateFlags],a
ucl_pressDown::
	ld a,[_PadDataEdge]
	bit 7,a
	jr z,ucl2
	ld hl,CurrentLine
	call LD_HLA_TO_BC
	inc bc
	call LD_BC_TO_HLA
	ld a,[StateFlags]
	or %00000010
	ld [StateFlags],a
ucl2::
	; clamp CurrentLine to [0, TotalLines-1]
	ld hl,CurrentLine
	call LD_HLA_TO_BC
	ld a,$ff
	cp b
	jr z,ucl3
	jp ucl5
ucl3::
	cp c
	jr z, ucl4
	jp ucl5
ucl4::
	ld bc,0
	call LD_BC_TO_HLA
ucl5::
	; Left and right (changes current choice)
	ld a,[StateFlags]
	and %11111011
	ld [StateFlags],a
	ld a,[_PadDataEdge]
	; left
	bit 5,a
	jp z,ucc1
	ld a,[StateFlags]
	or %000000100
	ld [StateFlags],a
	ld a,[CurrentChoice]
	sub 1
	jr c,ucc0_0
	jp ucc0
ucc0_0::
	ld a,[TotalChoices]
	dec a
ucc0::
	ld [CurrentChoice],a
ucc1::
	; right
	bit 4,a
	jp z,ucc3
	ld a,[StateFlags]
	or %000000100
	ld [StateFlags],a
	ld a,[CurrentChoice]
	inc a
	ld b,a
	ld a,[TotalChoices]
	cp b
	jr z, ucc2
	jp ucc2_0
ucc2::
	ld a,0
	jp ucc2_1
ucc2_0::
	ld a,b
ucc2_1::
	ld [CurrentChoice],a
ucc3::
	; A button (changes current passage)
	ld a,[StateFlags]
	and %11110111
	ld [StateFlags],a
	ld a,[_PadDataEdge]
	bit 0,a
	jp z,ucc4
	ld a,[StateFlags]
	or %00001000
	ld [StateFlags],a
ucc4::
	ret

LD_HLA_TO_BC::
	ld b,[hl]
	inc hl
	ld c,[hl]
	dec hl
	ret

LD_BC_TO_HLA::
	ld [hl],b
	inc hl
	ld [hl],c
	ret

UPDATE_CURSOR_BLINK::
	ld a,[StateFlags] ; turn off cursor flag
	and %11111110
	ld [StateFlags],a
	ld a,[CursorCounter]
	inc a
	ld b,a
	ld a,CURSOR_BLINK_MAX
	sub b
	jr nc,update_cursor_inrange
	ld b,0
update_cursor_inrange::
	ld a,b
	ld [CursorCounter],a
	ld a,CURSOR_BLINK
	sub b
	jr c,ucb1
	ld a,[StateFlags] ; if blinking, turn on cursor flag
	or %00000001
	ld [StateFlags],a
ucb1::
	call LOAD_CHOICE_VARS
	; test if choice line is on screen
	ld hl,TempChoiceLine
	ld b,[hl]
	inc hl
	ld c,[hl]
	call TEST_LINE_ONSCREEN
	or 0
	ret z ; return if line is not onscreen
	call GET_CHOICE_START_VRAM
	ld a,[StateFlags]
	bit 0,a
	jp z,ucb3
	call UPDATE_CURSOR_BLINK_OFF
	ret
ucb3::
	call UPDATE_CURSOR_BLINK_ON
	ret

UPDATE_CURSOR_BLINK_ON::
	ld b,0
	call SET_BYTE_VRAM
	inc hl
	inc e
	ld a,e
	cp 20
	jr z,ucb4
	jp ucb5
ucb4::
	ld e,0
	ld bc,12
	add hl,bc ; move vram to next line
ucb5::
	dec d
	jr nz,UPDATE_CURSOR_BLINK_ON
	ret

UPDATE_CURSOR_BLINK_OFF::
	push bc
	push hl
		ld b,0
		ld a,[TempChoiceLength]
		sub d
		ld c,a
		ld hl,ChoiceBuffer
		add hl,bc
		ld a,[hl]
	pop hl
	pop bc
	ld b,a
	call SET_BYTE_VRAM
	inc hl
	inc e
	ld a,e
	cp 20
	jr z,ucbo1
	jp ucbo2
ucbo1::
	ld e,0
	ld bc,12
	add hl,bc
ucbo2::
	dec d
	jr nz,UPDATE_CURSOR_BLINK_OFF
	ret

; Multiplies a by 6 into a.  Destroys b
MULTIPLY_6::
	ld b,a
	rlc b
	ld a,b
	rlc b
	add b
	ret

; Puts the new choice's line in bc without upsetting any BSS variables
GET_NEW_CHOICE_LINE::
	ld a,[CurrentChoice]
	call MULTIPLY_6
	ld b,0
	ld c,a
	ld hl,ChoiceStack
	add hl,bc
	ld a,[hl+] ; first term in choice stack is ChoiceLine, so we're good
	ld b,a
	ld a,[hl]
	ld c,a
	ret

MOVE_LINE_ONSCREEN::
; Moves current line of screen so that test_line appears on it
; Input:
; 	bc - test line
	; if test line < CurrentLine
	ld16r de,CurrentLine
	ld16rr hl,de
	ld a,b
	sub h
	jr nc, mlo1
mlo0::
	call MOVE_CURRENT_LINE_UP
	ret
mlo1::
	ld a,c
	sub l
	jr nc,mlo2
	jp mlo0
mlo2::
	; if test line > CurrentLine + 18
	ld de,18
	add hl,de
	ld a,h
	sub b
	jr nc, mlo4
mlo3::
	call MOVE_CURRENT_LINE_DOWN
	ret
mlo4::
	ld a,l
	sub c
	ret nc
	jp mlo3

MOVE_CURRENT_LINE_DOWN::
	; currentLine = testLine - 18
	ld hl,$ffef ; cpl 18 + 2?? MAGIC NUMBERS! :D :D :D
	add hl,bc
	ld16rr bc,hl
	ld16a CurrentLine,bc
	call UPDATE_SCREEN_LINES
	ret

MOVE_CURRENT_LINE_UP::
	; currentLine = testLine
	ld16a CurrentLine,bc
	call UPDATE_SCREEN_LINES
	ret

; sets a to if the given line is onscreen or not
; Input:
;	bc - test line
; Output:
; 	a - nz = onscreen
TEST_LINE_ONSCREEN::
	ld16r de,CurrentLine
	ld16rr hl,de
	ld a,b
	sub h
	jr nc,tlo1
tlo0::
	ld a,0
	ret
tlo1::
	ld a,c
	sub l
	jr nc, tlo2
	jp tlo0
tlo2::
	ld de,18
	add hl,de
	ld a,h
	sub b
	jr nc,tlo3
	jp tlo0
tlo3::
	ld a,l
	sub c
	jr nc, tlo4
	jp tlo0
tlo4::
	ld a,1
	ret

LOAD_CHOICE_VARS::
	ld a,[CurrentChoice]
	call MULTIPLY_6
	; add a to ChoiceStack
	ld b,0
	ld c,a
	ld hl,ChoiceStack
	add hl,bc
	; load necessary choice vars
	ld a,[hl+]
	ld [TempChoiceLine],a
	ld a,[hl+]
	ld [TempChoiceLine+1],a
	ld a,[hl+]
	ld [TempChoiceColumn],a
	ld a,[hl+]
	ld [TempChoiceLength],a
	ld a,[hl+]
	ld [TempChoicePassage],a
	ld a,[hl+]
	ld [TempChoicePassage+1],a
	ld a,[hl+]
	ret

; Loads vram choice address (choice vars must be loaded & current)
; Output:
; 	d - length
; 	e - column
; 	hl - vram address
GET_CHOICE_START_VRAM::
	; get number of lines from top (choiceline-curline)
	ld16r bc,CurrentLine
	cpl16 bc
	inc bc
	ld16r de,TempChoiceLine
	ld16rr hl,de
	add hl,bc
	ld c,l
	ld hl,_SCRN0
	ld de,32
	ld a,0
	cp c
	jr z,gcs2
gcs1::
	add hl,de
	dec c
	jr nz,gcs1
gcs2::
	; hl = choice vram line
	ld a,[TempChoiceColumn]
	ld e,a
	add hl,de
	; hl = choice vram column + line
	ld a,[TempChoiceLength]
	ld d,a
	ret

UPDATE_SCREEN_LINES::
	; set TempCurrentLine to CurrentLine
	ld16r bc,CurrentLine
	ld16a TempCurrentLine,bc
	; set TempPosition to _SCRN0
	ld bc,_SCRN0
	ld16a TempPosition,bc
	; set hl to current line LineStack pointer
	ld16r bc,CurrentLine
	ld hl,LineStack
	add hl,bc
	add hl,bc ; two bytes wide
usl1::
	; set bc to first character in line
	ld b,[hl]
	inc hl
	ld c,[hl]
	inc hl
	call UPDATE_SCREEN_LINE
	; if TempPosition == _SCRN0 + 32*18, return
	ld bc,TempPosition
	ld a,[bc]
	cp $9a
	jr z,usl2
	jp usl1
usl2::
	inc bc
	ld a,[bc]
	cp $40
	jr z,usl3
	jp usl1
usl3::
	ret

; Fill the line at TempPosition with spaces
; Destroys:
; 	Everything
FILL_LINE_EMPTY::
	ld16r de,TempPosition
	ld16rr hl,de
	ld b,$20
	ld c,20
fle1::
	call SET_BYTE_VRAM
	inc hl
	dec c
	jr nz,fle1
	ret

; Update the screen line hl from address bc
; Input
;	bc - line pointer
; Destroys
; 	a, bc, de
UPDATE_SCREEN_LINE::
	push hl
		; if TempCurrentLine > TotalLines, fill line with spaces
		ld16r de,TempCurrentLine
		ld16rr hl,de
		ld16r de,TotalLines
		dec de ; Hack because TotalLines is off by 2 for some reason
		dec de
		; de = TotalLines, hl = TempCurrentLine
		ld a,d
		sub h
		jr nc,usli0_0
		call FILL_LINE_EMPTY
		jp usli3
usli0_0::
		ld a,e
		sub l
		jr nc,usli0
		call FILL_LINE_EMPTY
		jp usli3
usli0::
		; otherwise fill line with line text
		ld16r de,TempPosition
		ld16rr hl,de
		ld d,20
usli1::
		ld a,[bc]
		push bc
			ld b,a
			call SET_BYTE_VRAM
		pop bc
		inc bc
		inc hl
		dec d
		jr nz,usli1
usli3::
		; move TempPosition to next line by adding 12
		ld de,12
		add hl,de
		ld16rr de,hl
		ld16a TempPosition,de
		; increment TempCurrentLine
		ld16r de,TempCurrentLine
		inc de
		ld16a TempCurrentLine,de
	pop hl
	ret

CLEAR_PASSAGE_RAM::
	push bc
	push hl
		ld a,0
		ld hl,ChoiceStack
		ld bc,252
		call CLEAR_RAM_CHUNK
		ld hl,LineStack
		ld bc,2048
		call CLEAR_RAM_CHUNK
		ld hl,CustomLineStack
		ld bc,640
		call CLEAR_RAM_CHUNK
		ld hl,ChoiceBuffer
		ld bc,256
		call CLEAR_RAM_CHUNK
	pop hl
	pop bc
	ret

; Clear a chunk of ram to 0
; Input
; 	a - must be 0
; 	bc - clear length
; 	hl - start address
; Destroys
; 	a, bc, hl
CLEAR_RAM_CHUNK::
	ld [hl+],a
	dec bc
	cp b
	jr z,crc1
	jp CLEAR_RAM_CHUNK
crc1::
	cp c
	ret z
	jp CLEAR_RAM_CHUNK


GET_CHOICE_START::
	; hl = [TempChoiceLine * 2 + LineStack] + TempChoiceColumn
	ld16r bc,TempChoiceLine
	ld hl,LineStack
	add hl,bc
	add hl,bc
	ld b,[hl]
	inc hl
	ld c,[hl]
	ld16rr hl,bc
	ld d,0
	ld a,[TempChoiceColumn]
	ld e,a
	add hl,de
	ld a,[TempChoiceLength]
	ld d,a
	ret


LOAD_CHOICE_BUFFER::
	call LOAD_CHOICE_VARS
	call GET_CHOICE_START
	; load dest
	ld bc,ChoiceBuffer
lcb1::
	ld a,[hl+]
	ld [bc],a
	inc bc
	inc e
	ld a,e
	cp 20
	jr z,lcb2
	jp lcb3
lcb2::
	push bc
		ld16r bc,TempChoiceLine
		inc bc
		ld16a TempChoiceLine,bc
		ld hl,LineStack
		add hl,bc
		add hl,bc
		ld b,[hl] ; TODO: fix the macro for this
		inc hl
		ld c,[hl]
		ld16rr hl,bc
	pop bc
lcb3::
	dec d
	jp nz,lcb1
	ret

UPDATE_SCREEN_PASSAGE::
	call LOAD_CHOICE_VARS
	ld a,[TempChoicePassage]
	ld h,a
	ld a,[TempChoicePassage+1]
	ld l,a
	call CLEAR_PASSAGE_RAM
	ld a,0
	or 111
	ld a,0
	call LOAD_PASSAGE
	call LOAD_CHOICE_BUFFER
	call UPDATE_SCREEN_LINES
	ret

; Load passage at address HL into memory
; Input
; 	a - nz: clear stacks
;	hl - passage address
LOAD_PASSAGE::
	or 0
	jr nz,ldps1
	; clear passage ram
	ld a,0
	ld [CurrentChoice],a
	ld [TotalChoices],a
	ld bc,TotalLines ; totalLines is a word
	ld [bc],a
	inc bc
	ld [bc],a
	ld bc,CurrentLine
	ld [bc],a
	inc bc
	ld [bc],a
ldps1::
	; swap hl bytes
	ld a,h
	ld h,l
	ld l,a
	; load choices into ChoiceStack
	ld b,[hl]
	inc hl
	push hl
		; set de to end of ChoiceStack
		ld a,[TotalChoices]
		ld hl,ChoiceStack
		ld de,6
		or 0
		jr z,ldps1_2
ldps1_1::
		add hl,de
		dec a
		jr nz,ldps1_1
ldps1_2::
		ld d,h
		ld e,l
	pop hl
ldps2::
	ld a,[TotalChoices]
	inc a
	ld [TotalChoices],a
	ld c,6
ldps3::
	ld a,[hl+]
	ld [de],a
	inc de
	dec c
	jr nz,ldps3
	dec b
	jr nz,ldps2
	; load lines into LineStack
	push hl
		; set de to end of LineStack
		ld hl,TotalLines
		ld b,[hl]
		inc hl
		ld c,[hl]
		ld hl,LineStack
		add hl,bc
		add hl,bc ; same as LineStack + TotalLines * 2
		ld d,h
		ld e,l
	pop hl
	ld bc,20
ldps4::
	push bc
	push hl
		; increment TotalLines
		ld hl,TotalLines
		ld b,[hl]
		inc hl
		ld c,[hl]
		inc bc
		dec hl
		ld [hl],b
		inc hl
		ld [hl],c
	pop hl
	pop bc
	ld a,[hl]
	cp $1a ; if line begins with EOL, return
	ret z
	call PARSE_COMMANDS
	cp 1
	jr z,ldps5
	ld a,h 	; if no commands were parsed, push passage line onto LineStack and move DE to next spot
	ld [de],a
	inc de
	ld a,l
	ld [de],a
	inc de
ldps5::
	add hl,bc
	jp ldps4

; Parse ascii-encoded twine commands
; NOTE: any parsed command should push DE forward 2
; Input:
; 	hl - 
; Output:
; 	a - nz if line was a command
PARSE_COMMANDS::
	push bc
	push de
	push hl
		ld a,0
	pop hl
	pop de
	pop bc
	ret

; Write b to VRAM location hl
; Input:
; 	b - VRAM data
; 	hl - VRAM address
; Destroys:
;	a
SET_BYTE_VRAM::
	ldh a,[$41]
	and 2
	jr nz,SET_BYTE_VRAM
	ld [hl],b
	ret
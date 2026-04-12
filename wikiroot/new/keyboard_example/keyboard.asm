INCLUDE "../../config/zconfig.def"
SECTION code_user

; Local keyboard library for zxnext_layer2_tilemap
; Reads Spectrum keyboard hardware and maintains key state

PUBLIC _scan_keyboard

; -----------------+---+---+---+---+---+
; BIT              | 4 | 3 | 2 | 1 | 0 |
; ------+----------+---+---+---+---+---+
; C= $FE|          |   |   |   |   |   |
; B= $F7|%11110111 | 5 | 4 | 3 | 2 | 1 |
;    $EF|%11101111 | 6 | 7 | 8 | 9 | 0 |
;    $FB|%11111011 | T | R | E | W | Q |
;    $DF|%11011111 | Y | U | I | O | P |
;    $FD|%11111101 | G | F | D | S | A |
;    $BF|%10111111 | H | J | K | L |ENT|
;    $FE|%11111110 | V | C | X | Z |SHF|
;    $7F|%01111111 | B | N | M |SYM|SPC|
; ------+----------+---|---+---+---+---+

_scan_keyboard:
	ld hl, keybuffer
	ld c, 0xFE  ; LSB port address

	ld b, keys_5_to_1
	call scan_half_row

	ld b, keys_6_to_0
	call scan_half_row

	ld b, keys_T_to_Q
	call scan_half_row

	ld b, keys_Y_to_P
	call scan_half_row

	ld b, keys_G_to_A
	call scan_half_row

	ld b, keys_H_to_ENT
	call scan_half_row

	ld b, keys_V_to_SHF
	call scan_half_row

	ld b, keys_B_to_SPC
	call scan_half_row
	ret

scan_half_row:
        in a, (c)
        xor $FF        ; flip the bits. so 1 is pressed
        and $1F        ; 00011111b -  are any bits on?
	jr z, nopress  ; no keys pressed

; start checking the 5 key press bits one by one...
; key5
	rr a           ; rotate right most bit into carry
	jr nc, not5    ; is the rightmost bit zero
	ld e,(hl)      ; it is one, so key must be pressed
	inc e          ; inc key strength
	jr z, key4     ; if we have gone over 255 dont store
	ld (hl),e      ; else store key strength
	jr key4
not5:
	ld (hl), 0     ; set to not pressed

; --------------------------
key4:
	inc l 		; next key
	rr a		; rotate keypress bits right
	jr nc, not4	; if rightmost bit 0 then nothing pressed
	ld e,(hl)	; else key pressed, load the current press strength
	inc e 		; increment key press strength (use for debounce)
	jr z, key3	; if we went over 255 then dont store it
	ld (hl),e 	; store press strength
	jr key3
not4:
	ld (hl), 0 	; not pressed. Turn key off

; --------------------------
key3:
	inc l
	rr a
	jr nc, not3
	ld e,(hl)
	inc e
	jr z, key2
	ld (hl),e
	jr key2
not3:
	ld (hl), 0

; --------------------------
key2:
	inc l
	rr a
	jr nc, not2
	ld e,(hl)
	inc e
	jr z, key1
	ld (hl),e
	jr key1
not2:
	ld (hl), 0

; --------------------------
key1:
	inc l
	rr a
	jr nc, not1
	ld e,(hl)
	inc e
	ret z
	ld (hl),e
	ret
not1:
	ld (hl), 0
	ret

nopress:
	ld (hl), a  ; set key press indicators for the half row to zero
		    ; using A as reg assign is faster (A is implicitly 0)
	inc l
	ld (hl), a
	inc l
	ld (hl), a
	inc l
	ld (hl), a
	inc l
	ld (hl), a
	inc l
	ret

; key row IO addresses
DEFC keys_5_to_1   = $F7
DEFC keys_6_to_0   = $EF
DEFC keys_T_to_Q   = $FB
DEFC keys_Y_to_P   = $DF
DEFC keys_G_to_A   = $FD
DEFC keys_H_to_ENT = $BF
DEFC keys_V_to_SHF = $FE
DEFC keys_B_to_SPC = $7F

; key press buffer - 40 bytes, one byte per key
keybuffer:
ZK_5:	db 0
ZK_4:	db 0
ZK_3:	db 0
ZK_2:	db 0
ZK_1:	db 0
; ----
ZK_6:	db 0
ZK_7:	db 0
ZK_8:	db 0
ZK_9:	db 0
ZK_0:	db 0
; ----
ZK_T:	db 0
ZK_R:	db 0
ZK_E:	db 0
ZK_W:	db 0
ZK_Q:	db 0
; ----
ZK_Y:	db 0
ZK_U:	db 0
ZK_I:	db 0
ZK_O:	db 0
ZK_P:	db 0
; ----
ZK_G:	db 0
ZK_F:	db 0
ZK_D:	db 0
ZK_S:	db 0
ZK_A:	db 0
; ----
ZK_H:	db 0
ZK_J:	db 0
ZK_K:	db 0
ZK_L:	db 0
ZK_ENT:	db 0
; ----
ZK_V:	db 0
ZK_C:	db 0
ZK_X:	db 0
ZK_Z:	db 0
ZK_SHF:	db 0
; ----
ZK_B:	db 0
ZK_N:	db 0
ZK_M:	db 0
ZK_SYM:	db 0
ZK_SPC:	db 0
; ----
; Expose the key buffer to C as a variable per key
; ----
PUBLIC _KEY5
DEFC _KEY5 = ZK_5
PUBLIC _KEY4
DEFC _KEY4 = ZK_4
PUBLIC _KEY3
DEFC _KEY3 = ZK_3
PUBLIC _KEY2
DEFC _KEY2 = ZK_2
PUBLIC _KEY1
DEFC _KEY1 = ZK_1
; ----
PUBLIC _KEY6
DEFC _KEY6 = ZK_6
PUBLIC _KEY7
DEFC _KEY7 = ZK_7
PUBLIC _KEY8
DEFC _KEY8 = ZK_8
PUBLIC _KEY9
DEFC _KEY9 = ZK_9
PUBLIC _KEY0
DEFC _KEY0 = ZK_0
; ----
PUBLIC _KEYT
DEFC _KEYT = ZK_T
PUBLIC _KEYR
DEFC _KEYR = ZK_R
PUBLIC _KEYE
DEFC _KEYE = ZK_E
PUBLIC _KEYW
DEFC _KEYW = ZK_W
PUBLIC _KEYQ
DEFC _KEYQ = ZK_Q
; ----
PUBLIC _KEYY
DEFC _KEYY = ZK_Y
PUBLIC _KEYU
DEFC _KEYU = ZK_U
PUBLIC _KEYI
DEFC _KEYI = ZK_I
PUBLIC _KEYO
DEFC _KEYO = ZK_O
PUBLIC _KEYP
DEFC _KEYP = ZK_P
; ----
PUBLIC _KEYG
DEFC _KEYG = ZK_G
PUBLIC _KEYF
DEFC _KEYF = ZK_F
PUBLIC _KEYD
DEFC _KEYD = ZK_D
PUBLIC _KEYS
DEFC _KEYS = ZK_S
PUBLIC _KEYA
DEFC _KEYA = ZK_A
; ----
PUBLIC _KEYH
DEFC _KEYH = ZK_H
PUBLIC _KEYJ
DEFC _KEYJ = ZK_J
PUBLIC _KEYK
DEFC _KEYK = ZK_K
PUBLIC _KEYL
DEFC _KEYL = ZK_L
PUBLIC _KEYENT
DEFC _KEYENT = ZK_ENT
; ----
PUBLIC _KEYV
DEFC _KEYV = ZK_V
PUBLIC _KEYC
DEFC _KEYC = ZK_C
PUBLIC _KEYX
DEFC _KEYX = ZK_X
PUBLIC _KEYZ
DEFC _KEYZ = ZK_Z
PUBLIC _KEYSHF
DEFC _KEYSHF = ZK_SHF
; ----
PUBLIC _KEYB
DEFC _KEYB = ZK_B
PUBLIC _KEYN
DEFC _KEYN = ZK_N
PUBLIC _KEYM
DEFC _KEYM = ZK_M
PUBLIC _KEYSYM
DEFC _KEYSYM = ZK_SYM
PUBLIC _KEYSPC
DEFC _KEYSPC = ZK_SPC
; ----

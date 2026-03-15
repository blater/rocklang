; Z80 assembly implementation of peek and poke for direct memory access
; Using z88dk calling convention with stack parameters

SECTION code_user

PUBLIC _peek
PUBLIC _poke

; uint8_t peek(uint16_t addr)
; Stack on entry: [SP]=return addr, [SP+2]=addr
; Return value in L (8-bit)
_peek:
    pop bc          ; BC = return address
    pop hl          ; HL = addr
    push bc         ; restore return address

    ld l, (hl)      ; Load byte from HL address
    ret

; void poke(uint16_t addr, uint8_t val)
; Stack on entry: [SP]=return addr, [SP+2]=address, [SP+4]=value
_poke:
    pop bc          ; BC = return address
    pop hl          ; HL = addr
    pop de          ; DE = val (val in E)
    push bc         ; restore return address

    ld (hl), e      ; Store E (val) to address in HL
    ret

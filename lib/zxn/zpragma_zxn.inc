; Changes the default value of the SP register
#pragma output REGISTER_SP = 0xBFF0

; Segment for banked calls:
; 0 : 0x0000 - 0x3FFF
; 1 : 0x4000 - 0x7FFF
; 2 : 0x8000 - 0xBFFF
; 3 : 0xC000 - 0xFFFF (default if no CLIB_BANKING_SEGMENT is provided)
#pragma output CLIB_BANKING_SEGMENT = 0

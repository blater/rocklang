// change program org
#pragma output CRT_ORG_CODE = 24500   // an alternative to -zorg on the compile line

// move the stack to available memory
// Stack below UDGs, safe conventional location in segment 3
// The -1 value is going to leave it where it is, which will be set by the basic loader. 
// Since we setup org code to 24500, the stack will be below that address.
#pragma output REGISTER_SP = 24481      // must be placed somewhere in the 16k ram

// halt program on exit, no need to play well with basic
#pragma output CRT_ON_EXIT = 0x10001

// eliminate the malloc heap
//#pragma output CLIB_MALLOC_HEAP_SIZE = 0

// eliminate the stdio heap (no files can be opened)
#pragma output CLIB_STDIO_HEAP_SIZE = 0

// eliminate unused printf converters
// placed used converters here, empty string if none (e.g. "%c %s %u %d")
#pragma printf = "%c %s %u %d"    


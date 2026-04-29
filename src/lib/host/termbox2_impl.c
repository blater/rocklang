/* Single translation unit that pulls in the termbox2 implementation.
 * Host (gcc) builds link this alongside any RTL component that needs
 * termbox2. Components themselves just #include "termbox2.h" without
 * defining TB_IMPL. Not compiled on ZXN.
 */

/* termbox2 is vendored as a single-header library; its static helpers
 * trip -Wunused-function in configurations that do not exercise every
 * code path. Suppress locally so the rest of the build stays -Wall clean. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#define TB_IMPL
#include "termbox2.h"

#pragma GCC diagnostic pop

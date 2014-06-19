#ifndef __TRY_H
#define __TRY_H

#include <windows.h>

#include "my_setjmp.h"

void manage_handler();

extern my_jmp_buf g_env;

#define __try \
	if (my_setjmp(g_env) != 0) { \
		goto __try_exception_catched; \
	} \
	set_handler();

#define __except \
	goto __try_finally; \
__try_exception_catched: \
	restore_handler(); \
__try_finally:

#endif // __TRY_H
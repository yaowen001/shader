//-----------------------------------------------------------------------------
#ifndef __APP_H
#define __APP_H

//-----------------------------------------------------------------------------
#include    "app_context.h"
#include    "app_window.h"
//-----------------------------------------------------------------------------
#ifndef null
	#define null 0
#endif

#ifndef true
	#define true 1
#endif

#ifndef false
	#define false 0
#endif

typedef unsigned int bool;
//-----------------------------------------------------------------------------
#ifdef  __cplusplus
#define DEF_EXTERN  extern "C"
#else
#define DEF_EXTERN  extern
#endif
//-----------------------------------------------------------------------------
DEF_EXTERN 	void    app_set_quit(void);
DEF_EXTERN 	bool    app_run(void);
//-----------------------------------------------------------------------------

#endif // __APP_H
//-----------------------------------------------------------------------------

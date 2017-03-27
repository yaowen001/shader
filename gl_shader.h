//-----------------------------------------------------------------------------
#ifndef __GL_SHADER_H
#define __GL_SHADER_H

//-----------------------------------------------------------------------------
#include    "gl_shader2d.h"
#include    "gl_shader2d_ttf.h"
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

typedef   signed char	s8bits;
typedef   signed short 	s16bits;
typedef   signed int 	s32bits;

typedef unsigned char 	u8bits;
typedef unsigned short 	u16bits;
typedef unsigned int  	u32bits;

typedef unsigned int bool;
//-----------------------------------------------------------------------------
#ifdef  __cplusplus
#define DEF_EXTERN  extern "C"
#else
#define DEF_EXTERN  extern
#endif
//-----------------------------------------------------------------------------
DEF_EXTERN	bool	gl_shader_init(u32bits width, u32bits height);
DEF_EXTERN	void	gl_shader_end(void);
//-----------------------------------------------------------------------------

#endif // __GL_SHADER_H
//-----------------------------------------------------------------------------

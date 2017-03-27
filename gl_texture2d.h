//-----------------------------------------------------------------------------
#ifndef __GL_TEXTURE2D_H
#define __GL_TEXTURE2D_H

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
typedef struct  tagTEXTURE2D
{
    u32bits  	iw, ih; // 貼圖的圖像寬高
	
    u32bits  	tw, th; // 貼圖的寬高
	
    float   	tl, tu, tr, td; // 貼圖的 uv 座標

    u32bits  	texID;	// 貼圖的 ID
	
	u32bits     size;	// 貼圖佔用的容量
} TEXTURE2D;

// 目前只支持 DDS 檔、TGA 檔
DEF_EXTERN TEXTURE2D	*gl_texture2d_load_memory_file(u8bits *file_buf, u32bits buf_size, bool linear);
DEF_EXTERN TEXTURE2D	*gl_texture2d_load_file(char *file_name, bool linear);
DEF_EXTERN void    		gl_texture2d_free(TEXTURE2D *texture);

#endif // __GL_TEXTURE2D_H
//-----------------------------------------------------------------------------

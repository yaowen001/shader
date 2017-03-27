//-----------------------------------------------------------------------------
#ifndef __GL_SHADER2D_TTF_H
#define __GL_SHADER2D_TTF_H

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
DEF_EXTERN	bool	gl_shader2d_ttf_init(void);
DEF_EXTERN	void	gl_shader2d_ttf_end(void);
//-----------------------------------------------------------------------------
typedef struct  tagTTF_GLYPH_INFO
{
	u32bits		char_code; // 字碼
	
    float		tl, tu, tr, td; // 貼圖的 uv 座標
    u32bits  	texID; // 貼圖的 ID
	
    u32bits  	tmp_iw, tmp_ih; // 暫存 BITMAP 圖像的寬高	
	u32bits		*tmp_bitmap; // 暫存 BITMAP 圖像資料指標
	
    u32bits  	draw_iw, draw_ih;	
    u32bits  	draw_texID; // 用來繪圖的貼圖ID
	
	s32bits		x_offset, y_offset; // 繪圖偏移值
	u32bits		x_advance, y_advance; // 到下一個字的繪圖偏移值	
} TTF_GLYPH_INFO;

#define	DEF_TTF_MAX_GLYPTH_CACHE_ID	(0xFF)

typedef	struct	tagTTF_FONT
{
	void 			*face;
	
	TTF_GLYPH_INFO	glyph_info[DEF_TTF_MAX_GLYPTH_CACHE_ID + 1];	
    u32bits  		tw, th; // 貼圖的寬高(設定的字型寬高)
	
	u32bits			*tmp_buf; // 暫存 BITMAP 圖像資料指標
    u32bits  		tmp_tw, tmp_th;

    u32bits  		texID; // 貼圖的 ID
	
	u32bits     	size;	// 全部貼圖佔用的容量
	
    s32bits         ascender;
    s32bits         descender;
	s32bits			height;
} TTF_FONT;
//-----------------------------------------------------------------------------
// 只支持 UNICODE 編碼，而且字型可縮放 的字型檔
DEF_EXTERN	TTF_FONT	*gl_shader2d_ttf_open_file(char *ttf_file, u32bits size);
DEF_EXTERN 	void 		gl_shader2d_ttf_close_file(TTF_FONT *ttf_font);
DEF_EXTERN 	void		gl_shader2d_ttf_pixel_size(TTF_FONT *ttf_font, u32bits *w, u32bits *h, wchar_t *string);
DEF_EXTERN 	void 		gl_shader2d_ttf_draw_glyph(TTF_FONT *ttf_font, s32bits x, s32bits y, u8bits r, u8bits g, u8bits b, u8bits a, wchar_t *szFormat, ...);
//-----------------------------------------------------------------------------

#endif // __GL_SHADER2D_TTF_H
//-----------------------------------------------------------------------------

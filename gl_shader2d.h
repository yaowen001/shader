//-----------------------------------------------------------------------------
#ifndef __GL_SHADER2D_H
#define __GL_SHADER2D_H

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
DEF_EXTERN bool 	gl_shader2d_init(u32bits w, u32bits h);
DEF_EXTERN void		gl_shader2d_end(void);
//-----------------------------------------------------------------------------
DEF_EXTERN void    gl_shader2d_clip_on(s32bits x, s32bits y, u32bits w, u32bits h);
DEF_EXTERN void    gl_shader2d_clip_off(void);
//-----------------------------------------------------------------------------
DEF_EXTERN void    gl_shader2d_draw_begin(void);
DEF_EXTERN void    gl_shader2d_draw_end(void);
//-----------------------------------------------------------------------------
DEF_EXTERN void    gl_shader2d_background_color_set(u8bits r, u8bits g, u8bits b);
DEF_EXTERN void    gl_shader2d_clear(void);
//-----------------------------------------------------------------------------
DEF_EXTERN void    gl_shader2d_draw_pixel(s32bits x, s32bits y, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_draw_line(s32bits x1, s32bits y1, s32bits x2, s32bits y2, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_draw_rect(s32bits x1, s32bits y1, s32bits x2, s32bits y2, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_draw_rect_fill(s32bits x1, s32bits y1, s32bits x2, s32bits y2, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_draw_circle(s32bits xc, s32bits yc, s32bits rc, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_draw_circle_fill(s32bits xc, s32bits yc, s32bits rc, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_draw_ellipse(s32bits xc, s32bits yc, s32bits ra, s32bits rb, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_draw_ellipse_fill(s32bits xc, s32bits yc, s32bits ra, s32bits rb, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void    gl_shader2d_printf(s32bits x, s32bits y, u8bits r, u8bits g, u8bits b, u8bits a, char *szFormat, ...); // only ascii
//-----------------------------------------------------------------------------
#define DEF_USE_PROGRAM_TYPE_NULL                   (0)
#define DEF_USE_PROGRAM_TYPE_DEFAULT                (1)
#define DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR   (2)
#define DEF_USE_PROGRAM_TYPE_ASSIGN_COLOR           (3)
#define DEF_USE_PROGRAM_TYPE_NEGATIVE_COLOR         (4)
#define DEF_USE_PROGRAM_TYPE_GRAY_COLOR             (5)

DEF_EXTERN void    gl_shader2d_draw_texture(u32bits program_type, 
											s32bits x, s32bits y, u32bits w, u32bits h,
											u8bits r, u8bits g, u8bits b, u8bits a,
											bool h_mirror, bool v_mirror,
											s32bits angle,
											u32bits texID, float tl, float tu, float tr, float td);
DEF_EXTERN void    gl_shader2d_assign_color_set(u8bits r, u8bits g, u8bits b);
//-----------------------------------------------------------------------------
DEF_EXTERN void    gl_shader2d_color_set(u32bits color);
DEF_EXTERN void    gl_shader2d_move_to(s32bits x, s32bits y);
DEF_EXTERN void	   gl_shader2d_draw_line_to(s32bits x, s32bits y);
//-----------------------------------------------------------------------------
typedef struct  tagRENDER_TEXTURE
{
    u32bits     idOldFBO; // FBO 貼圖的舊 ID
    u32bits     idFBO; // FBO 貼圖的 ID
    //u32bits     idRBO; // RBO 貼圖的 ID
    
    u32bits  	iw, ih; // 貼圖的圖像寬高

    u32bits     tw, th; // 貼圖的寬高
	
    float   	tl, tu, tr, td; // 貼圖的 uv 座標
	
    u32bits  	texID; // 貼圖的 ID

    u32bits     size; // 貼圖佔用的容量
} RENDER_TEXTURE;
    
DEF_EXTERN RENDER_TEXTURE	*gl_shader2d_fbo_alloc(u32bits w, u32bits h);
DEF_EXTERN void            	gl_shader2d_fbo_free(RENDER_TEXTURE *rt);
DEF_EXTERN void            	gl_shader2d_fbo_draw_begin(RENDER_TEXTURE *rt, u8bits r, u8bits g, u8bits b, u8bits a);
DEF_EXTERN void            	gl_shader2d_fbo_draw_end(RENDER_TEXTURE *rt);
DEF_EXTERN void            	gl_shader2d_fbo_draw_texture(RENDER_TEXTURE *rt,
														 u32bits program_type,
														 s32bits x, s32bits y,
														 u32bits w, u32bits h,
														 u8bits r, u8bits g, u8bits b, u8bits a, 
														 bool h_mirror, bool v_mirror,
														 s32bits angle);
DEF_EXTERN void            	gl_shader2d_fbo_texture_pixel_get(RENDER_TEXTURE *rt, u8bits *buffer, u32bits x, u32bits y, u32bits w, u32bits h);
//-----------------------------------------------------------------------------

#endif // __GL_SHADER2D_H
//-----------------------------------------------------------------------------

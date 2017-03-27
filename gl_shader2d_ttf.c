//-----------------------------------------------------------------------------
#include    <stdio.h>
#include    <stdlib.h>
#include    <memory.h>
#include    <math.h>
#include    <GL/glew.h>

#include    "gl_shader2d.h"
#include    "gl_shader2d_ttf.h"
//-----------------------------------------------------------------------------
static  bool    non_power_of_two;
//-----------------------------------------------------------------------------
#include	<ft2build.h>
#include 	FT_FREETYPE_H
//-----------------------------------------------------------------------------
static 	FT_Library	ttf_library;
static 	bool		ttf_init_ok;
//-----------------------------------------------------------------------------
bool 	gl_shader2d_ttf_init(void)
{
	non_power_of_two = true;
	
	ttf_init_ok = false;	
	
	if(FT_Init_FreeType(&ttf_library) != 0)
	{
		return false;
	}
	
	ttf_init_ok = true;
	return true;
}
//-----------------------------------------------------------------------------
void 	gl_shader2d_ttf_end(void)
{
	if(ttf_init_ok)
	{
		FT_Done_FreeType(ttf_library);
	}
}
//-----------------------------------------------------------------------------
static 	bool	gl_shader2d_ttf_texture_create(TTF_FONT *ttf_font, u32bits width, u32bits height)
{
	TTF_GLYPH_INFO	*glyph_info;
	u32bits			texID;
	u32bits			tw, th, size;
	float 			u, v;
	u32bits			i;
	
	if(non_power_of_two)
	{
		tw = width;
		th = height;
	}
	else
	{    
		tw = 1;
		th = 1;
		while(tw < width) { tw <<= 1; }
		while(th < height) { th <<= 1; }
	}
	
    u = (float)width / (float)tw;
    v = (float)height / (float)th;

	size = tw * th * sizeof(u32bits);
	
	ttf_font->tmp_buf = (u32bits *)malloc(size);
	if(ttf_font->tmp_buf == null)
	{
		return false;
	}
	
	ttf_font->tw   = tw;	
	ttf_font->th   = th;
	ttf_font->size = size * (DEF_TTF_MAX_GLYPTH_CACHE_ID + 1);
	
	for(i=0; i<=DEF_TTF_MAX_GLYPTH_CACHE_ID; i++)
	{
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);	
		
		glyph_info = &ttf_font->glyph_info[i];
		
		glyph_info->char_code = 0;
		
		glyph_info->tl        = 0.0f;
		glyph_info->tu        = 0.0f;
		glyph_info->tr        = u;
		glyph_info->td        = v;
		glyph_info->texID     = texID;
		
		glyph_info->tmp_iw     = tw;
		glyph_info->tmp_ih     = th;
		glyph_info->tmp_bitmap = null;
		
		glyph_info->x_offset  = 0;
		glyph_info->y_offset  = 0;
		glyph_info->x_advance = 0;
		glyph_info->y_advance = 0;
	}
	
	ttf_font->tmp_tw = tw << 1;
	ttf_font->tmp_th = th << 1;
	
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ttf_font->tmp_tw, ttf_font->tmp_th, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);	
	ttf_font->texID = texID;
	
	ttf_font->size += (ttf_font->tw * ttf_font->th * sizeof(u32bits));
		
	return true;
}
//-----------------------------------------------------------------------------
static 	void	gl_shader2d_ttf_texture_destroy(TTF_FONT *ttf_font)
{
	TTF_GLYPH_INFO	*glyph_info;
	u32bits			i;
	
	for(i=0; i<=DEF_TTF_MAX_GLYPTH_CACHE_ID; i++)
	{
		glyph_info = &ttf_font->glyph_info[i];
		
		if(glIsTexture(glyph_info->texID))
		{
			glDeleteTextures(1, (const GLuint*)&glyph_info->texID);
			if(glGetError() != GL_NO_ERROR)
			{
				printf("glGetError:glDeleteTextures\n");
			}			
		}
		
		if(glyph_info->tmp_bitmap)
		{
			free(glyph_info->tmp_bitmap);
			glyph_info->tmp_bitmap = null;
		}	
	}
	
	if(glIsTexture(ttf_font->texID))
	{
		glDeleteTextures(1, (const GLuint*)&ttf_font->texID);
		if(glGetError() != GL_NO_ERROR)
		{
			printf("glGetError:glDeleteTextures\n");
		}			
	}
	
	if(ttf_font->tmp_buf)
	{
		free(ttf_font->tmp_buf);
		ttf_font->tmp_buf = null;
	}
}
//-----------------------------------------------------------------------------
#define DEF_MAX_TTF_TEXTURE_SIZE	(128)

TTF_FONT	*gl_shader2d_ttf_open_file(char *ttf_file, u32bits size)
{
	TTF_FONT		*ttf_font;
	FT_Error 		error;
	FT_Face			face;
    FT_CharMap 		char_map;
	bool			found;
	s32bits 		i;
		
	if((size == 0) || (size > DEF_MAX_TTF_TEXTURE_SIZE)) return null;
	
	ttf_font = (TTF_FONT *)malloc(sizeof(TTF_FONT)); // 配置記憶體 用來記錄相關資料
	if(ttf_font == null)
	{
		return null;
	}
	memset(ttf_font, 0, sizeof(TTF_FONT));

	if(gl_shader2d_ttf_texture_create(ttf_font, size, size) == false)
	{
		free(ttf_font);
		return null;
	}
	
	error = FT_New_Face(ttf_library, ttf_file, 0, &((FT_Face)(ttf_font->face))); // 開啟字型檔
	if(error)
	{	
		gl_shader2d_ttf_texture_destroy(ttf_font);
		free(ttf_font);
		return null;
	}
	
	face = (FT_Face)ttf_font->face;
	
	// 檢查  是否可縮放
	if((face->face_flags & FT_FACE_FLAG_SCALABLE) != FT_FACE_FLAG_SCALABLE) 
	{
		FT_Done_Face((FT_Face)ttf_font->face);
		gl_shader2d_ttf_texture_destroy(ttf_font);
		free(ttf_font);
		return null;
	}
	
	// 檢查  需有支持 UNICODE 的編碼
	found = false;
    for(i=0; i<face->num_charmaps; i++) 
	{
        char_map = face->charmaps[i];
		if(char_map->encoding == 0x756e6963) // unic
		{
			found = true;
			break;
		}
    }
    if(found == false) 
	{
		FT_Done_Face((FT_Face)ttf_font->face);
		gl_shader2d_ttf_texture_destroy(ttf_font);
		free(ttf_font);
		
		return null;
    }
	
	FT_Select_Charmap(face, FT_ENCODING_UNICODE); // 選擇 UNICODE 編碼
	
	FT_Set_Pixel_Sizes(face, size, size); // 設定字型像點寬高
	
	ttf_font->ascender = (face->size->metrics.ascender) >> 6;
	ttf_font->descender = (face->size->metrics.descender) >> 6;
	ttf_font->height = (face->size->metrics.height) >> 6;
	
	return ttf_font;
}
//-----------------------------------------------------------------------------
void	gl_shader2d_ttf_close_file(TTF_FONT *ttf_font)
{
	if(ttf_font == null) return;
	
	if(ttf_font->face)
	{
		FT_Done_Face((FT_Face)ttf_font->face);
		ttf_font->face = null;
	}
	
	gl_shader2d_ttf_texture_destroy(ttf_font);
	
	free(ttf_font);
}
//-----------------------------------------------------------------------------
static 	TTF_GLYPH_INFO	*gl_shader2d_ttf_cache_check(TTF_FONT *ttf_font, u32bits c_code)
{
	FT_Face				face;
	TTF_GLYPH_INFO		*glyph_info;
	FT_UInt				c_index;
	FT_GlyphSlot		glyph_slot;
	FT_Glyph_Metrics  	*metrics;
	FT_Bitmap         	*bitmap;
	u32bits				x, y;
	u8bits				*src;
	u32bits 			*dst;
    u32bits  			next_offset;
	u32bits 			texID, tw, th, *tmp_buf;
	u32bits				size;
	bool				check_free;

	glyph_info = &ttf_font->glyph_info[c_code & DEF_TTF_MAX_GLYPTH_CACHE_ID];
	
	if(glyph_info->char_code != c_code) // 比較記錄的 char_code 是不是一樣
	{
		face = (FT_Face)ttf_font->face;
		
		c_index = FT_Get_Char_Index(face, c_code); // 取得字型索引
		FT_Load_Glyph(face, c_index, FT_LOAD_RENDER); // 載入字型並繪製
		
		glyph_slot = face->glyph;
		metrics = &glyph_slot->metrics;
		bitmap = &glyph_slot->bitmap;

		if((bitmap->width <= ttf_font->tw) && (bitmap->rows <= ttf_font->th))
		{	
			dst 		= ttf_font->tmp_buf;
			next_offset = ttf_font->tw;
			
			texID 	= glyph_info->texID;
			tw 		= ttf_font->tw;
			th 		= ttf_font->th;
			tmp_buf = ttf_font->tmp_buf;
			
			glyph_info->draw_iw = bitmap->width;
			glyph_info->draw_ih = bitmap->rows;
			
			glyph_info->tr = (float)glyph_info->draw_iw / (float)ttf_font->tw;
			glyph_info->td = (float)glyph_info->draw_ih / (float)ttf_font->th;
		
		    glyph_info->draw_texID = glyph_info->texID;
		}
		else
		{
			check_free = false;
			if(bitmap->width > glyph_info->tmp_iw) { glyph_info->tmp_iw = bitmap->width; check_free = true; }
			if(bitmap->rows > glyph_info->tmp_ih){ glyph_info->tmp_ih = bitmap->rows; check_free = true; }			
			if(check_free)
			{
				if(glyph_info->tmp_bitmap) // 如果先前有配置暫存 BITMAP 記憶體就要釋放
				{
					free(glyph_info->tmp_bitmap);
					glyph_info->tmp_bitmap = null;
				}
			}
			if(glyph_info->tmp_bitmap == null)
			{
				// 配置暫存 BITMAP 的記憶體
				size = glyph_info->tmp_iw * glyph_info->tmp_ih * sizeof(u32bits);
				glyph_info->tmp_bitmap = (u32bits *)malloc(size);
				if(glyph_info->tmp_bitmap == null)
				{
					return null;
				}
				//memset(glyph_info->tmp_bitmap, 0, size);
			}
		
			dst 		= glyph_info->tmp_bitmap;
			next_offset = glyph_info->tmp_iw;
			
			texID 	= ttf_font->texID;
			tw 		= glyph_info->tmp_iw;
			th 		= glyph_info->tmp_ih;
			tmp_buf = glyph_info->tmp_bitmap;
			
			glyph_info->draw_iw = bitmap->width;
			glyph_info->draw_ih = bitmap->rows;

			glyph_info->tr = (float)glyph_info->draw_iw / (float)ttf_font->tmp_tw;
			glyph_info->td = (float)glyph_info->draw_ih / (float)ttf_font->tmp_tw;
			
		    glyph_info->draw_texID = ttf_font->texID;
		}
		
		src = bitmap->buffer;
		for(y=0; y<bitmap->rows; y++)
		{
			for(x=0; x<bitmap->width; x++)
			{
				if(*(src + x))
				{			
					*(dst + x) = 0xFFFFFFFF;
				}
				else
				{
					*(dst + x) = 0x00000000;
				}
			}
			src += bitmap->pitch;
			dst += next_offset;
		}
		
		// 更新貼圖
		glBindTexture(GL_TEXTURE_2D, (GLuint)texID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, tmp_buf);
		
		glyph_info->char_code = c_code;
		
		glyph_info->x_offset = glyph_slot->bitmap_left;
		glyph_info->y_offset = ttf_font->ascender - glyph_slot->bitmap_top;
		glyph_info->x_advance = (metrics->horiAdvance >> 6);
		glyph_info->y_advance = (metrics->vertAdvance >> 6);
	}
	
	return glyph_info;
}
//-----------------------------------------------------------------------------
static 	TTF_GLYPH_INFO	*gl_shader2d_ttf_draw_texture(s32bits x, s32bits y, 
													  u8bits r, u8bits g, u8bits b, u8bits a, 
													  TTF_FONT *ttf_font, u32bits c_code)
{
	TTF_GLYPH_INFO	*glyph_info;

	glyph_info = gl_shader2d_ttf_cache_check(ttf_font, c_code);
	if(glyph_info == null)
	{
		return null;
	}
	
	gl_shader2d_draw_texture(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR,
							 x + glyph_info->x_offset, y + glyph_info->y_offset, glyph_info->draw_iw, glyph_info->draw_ih,
							 r, g, b, a,
							 false, false,
							 0,
							 glyph_info->draw_texID, glyph_info->tl, glyph_info->tu, glyph_info->tr, glyph_info->td);
		
	return glyph_info;
}
//-----------------------------------------------------------------------------
void	gl_shader2d_ttf_pixel_size(TTF_FONT *ttf_font, u32bits *w, u32bits *h, wchar_t *string)
{
	TTF_GLYPH_INFO	*glyph_info;
	u32bits			c_code;
	u32bits			c_len;
	u32bits 		i;	
	u32bits			iw, ih;
	
	iw = 0;
	ih = 0;
 	c_len = wcslen(string);
	for(i=0; i<c_len; i++)
	{
		c_code = string[i]; // 字碼
		
		if(c_code == 0x0020) // 空白
		{
			iw++;
			continue;
		}
	
		glyph_info = gl_shader2d_ttf_cache_check(ttf_font, c_code);
		if(glyph_info == null)
		{
			iw += ttf_font->tw;
		}
		else
		{
			iw += glyph_info->x_advance;
			if((glyph_info->y_offset + glyph_info->y_advance) > ih)
			{
				ih = glyph_info->y_offset + glyph_info->y_advance;;
			}
		}
	}
	
	if(w)
	{
		*w = iw;
	}
	if(h)
	{
		*h = ih;
	}
}
//-----------------------------------------------------------------------------
static	wchar_t	szText[256];

void 	gl_shader2d_ttf_draw_glyph(TTF_FONT *ttf_font, s32bits x, s32bits y, u8bits r, u8bits g, u8bits b, u8bits a, wchar_t *szFormat, ...)
{
    va_list         args;
	TTF_GLYPH_INFO	*glyph_info;
	u32bits			c_code;
	u32bits			c_len;
	u32bits 		i;	
    
    va_start(args, szFormat);
#if defined(WIN32)
    vswprintf_s((wchar_t *)szText, sizeof(szText), (wchar_t *)szFormat, args);
#else    
    vswprintf((wchar_t *)szText, (wchar_t *)szFormat, args);
#endif
    va_end(args);
    
 	c_len = wcslen(szText);
	for(i=0; i<c_len; i++)
	{
		c_code = szText[i]; // 字碼
		
		if(c_code == 0x0020) // 空白
		{
			x++;
			continue;
		}
		
		glyph_info = gl_shader2d_ttf_draw_texture(x, y, r, g, b, a, ttf_font, c_code);
		if(glyph_info == null)
		{
			x += ttf_font->tw;
		}
		else
		{
			x += glyph_info->x_advance;			
		}
	}
}
//-----------------------------------------------------------------------------
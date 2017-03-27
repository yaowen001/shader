//-----------------------------------------------------------------------------
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <GL/glew.h>

#include    "gl_texture2d.h"
//-----------------------------------------------------------------------------
static 	bool	non_power_of_two;
//-----------------------------------------------------------------------------
#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
//-----------------------------------------------------------------------------
static	TEXTURE2D	*gl_texture2d_read_dds_memory_file(u8bits *file_buf, u32bits buf_size, bool linear)
{
    u8bits      *pFileMemory;
    u8bits      *ddsTag;
    u8bits		*ddsHead;
    u32bits     width, height, linear_size, mipmap_count, fourCC;
    u8bits      *data;
    u32bits     data_size;
    u32bits     components;
    u32bits     format;
    GLuint      texID;
    u32bits     iw, ih, tw, th, size;
    float       u, v;
    TEXTURE2D	*texture;
    u32bits     blockSize;
    u32bits     offset;
    u32bits     level;
    
    
    pFileMemory = (u8bits *)file_buf;
    
    ddsTag = (u8bits *)pFileMemory;
    if(ddsTag[0] != 'D' || ddsTag[1] != 'D' || ddsTag[2] != 'S' || ddsTag[3] != ' ')
    {
        return null;
    }
    
    ddsHead = (u8bits *)(pFileMemory + 4);
    height = *(u32bits *)(ddsHead + 8);
    width = *(u32bits *)(ddsHead + 12);
    linear_size = *(u32bits *)(ddsHead + 16);
    mipmap_count = *(u32bits *)(ddsHead + 24);
    fourCC = *(u32bits *)(ddsHead + 80);
    
    
    iw = width;
    ih = height;
    
    if(non_power_of_two)
    {
        tw = iw;
        th = ih;
    }
    else
    {
        tw = 1;
        th = 1;
        while(tw < iw) { tw <<= 1; }
        while(th < ih) { th <<= 1; }
    }
    
    u = (float)iw / (float)tw;
    v = (float)ih / (float)th;
    
    // how big is it going to be including all mipmaps?
    data_size = mipmap_count > 1 ? linear_size * 2 : linear_size;
    data = (u8bits *)(pFileMemory + 4 + 124);
    
    components = (fourCC == FOURCC_DXT1) ? 3 : 4;
    switch(fourCC)
    {
        case FOURCC_DXT1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case FOURCC_DXT3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case FOURCC_DXT5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        default:
            return null;
    }
    
    blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    offset = 0;
    level = 0;

    texture = (TEXTURE2D *)malloc(sizeof(TEXTURE2D));
    if(texture == null)
    {
        return null;
    }
		
	texture->iw           = iw;
	texture->ih           = ih;
	texture->tw           = tw;
	texture->th           = th;
	texture->tl           = 0.0f;
	texture->tu           = 0.0f;
	texture->tr           = u;
	texture->td           = v;
	//texture->texID        = texID;
	
    glGenTextures(1, &texture->texID);
    glBindTexture(GL_TEXTURE_2D, texture->texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(linear)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    
    #if 0 // no use mipmap
    for(level=0; level<mipmap_count && (width || height); ++level)
    {
        size = ((tw + 3) / 4) * ((th + 3) / 4) * blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, tw, th, 0, size, data + offset);
        
        offset += size;
        tw = max(tw/2, 1);
        th = max(th/2, 1);  
    }
    #else
    size = ((tw + 3) / 4) * ((th + 3) / 4) * blockSize;
    glCompressedTexImage2D(GL_TEXTURE_2D, level, format, tw, th, 0, size, data + offset); // main memory -> graphics memory
    #endif
    
	texture->size  		  = size;
	
	return texture;
}
//-----------------------------------------------------------------------------
static	TEXTURE2D 	*gl_texture2d_read_tga_memory_file(u8bits *file_buf, u32bits buf_size, bool linear)
{
    u8bits		tgaIDLength;
    u8bits      tgaColormapType;
    u8bits      tgaImageType;
    u8bits      tgaColormapSpecification[5];
    //u16bits     tgaXOrigin;
    //u16bits     tgaYOrigin;
    u16bits     tgaImageWidth;
    u16bits     tgaImageHeight;
    u8bits      tgaPixelDepth;
    u8bits      tgaImageDescriptor;
    u32bits     dwSizeImage;
    s32bits 	x, y, i;
    u8bits      *pbits, *pByte;
    u8bits      r, g, b, a;
    u8bits      PacketInfo;
    u16bits     PacketType;
    u16bits     PixelCount;
    u32bits     dwWidth, dwHeight, dwDepth;
    u32bits     *psrc, *pdst, *pline;
    u8bits      *s;

    TEXTURE2D	*texture;
    GLuint      texID;
    u32bits     iw, ih, tw, th;
    u32bits     size;
    float       u, v;

    s = file_buf;

    /* 讀取檔頭 */
    tgaIDLength = *s;
    s++;
    tgaColormapType = *s;
    if(tgaColormapType != 0) return null; /* 只提供 24 或 32 bits 影像格式 */
    s++;

    tgaImageType = *s;
    s++;
    if((tgaImageType != 2 && tgaImageType != 10)) return null; /* 只提供 type 2 或 10 的 RGB 影像格式 */

    tgaColormapSpecification[0] = *s;
    s++;
    tgaColormapSpecification[1] = *s;
    s++;
    tgaColormapSpecification[2] = *s;
    s++;
    tgaColormapSpecification[3] = *s;
    s++;
    tgaColormapSpecification[4] = *s;
    s++;
    //tgaXOrigin = *((u16bits *)s);
    s += 2;
    //tgaYOrigin = *((u16bits *)s);
    s += 2;
    tgaImageWidth = *((u16bits *)s);
    s += 2;
    tgaImageHeight = *((u16bits *)s);
    s += 2;

    tgaPixelDepth = *s;
    if(tgaPixelDepth != 24 && tgaPixelDepth != 32) return null; /* 只提供 24 或 32 bits 影像格式 */
    s++;

    tgaImageDescriptor = *s;
    s++;

    // Skip the ID field. The first byte of the header is the length of this field
    if(tgaIDLength) s += tgaIDLength;

    iw = tgaImageWidth; /* 圖寬 */
    ih = tgaImageHeight; /* 圖高 */

    if(non_power_of_two)
    {
        tw = iw;
        th = ih;
    }
    else
    {
        tw = 1;
        th = 1;
        while(tw < iw) { tw <<= 1; }
        while(th < ih) { th <<= 1; }
    }
	
    u = (float)iw / (float)tw;
    v = (float)ih / (float)th;

    size = sizeof(u32bits) * tw * th;

    /* 圖像 */
    dwSizeImage = tgaImageWidth * tgaImageHeight * sizeof(u32bits);
    pbits = (u8bits *)malloc(dwSizeImage);
    if(pbits == null) return null;
    memset(pbits, 0, dwSizeImage);

    dwWidth  = (u32bits)tgaImageWidth;
    dwHeight = (u32bits)tgaImageHeight;
    dwDepth  = (u32bits)tgaPixelDepth;

    if(tgaImageType == 2) /* 未壓縮，RGB 影像 */
    {
        //psrc = (u32bits *)pbits + (dwWidth * (dwHeight - 1));
        //pByte = (u8bits *)psrc;
        for(y=0; y<(s32bits)dwHeight; y++)
        {
            pByte = pbits + y * dwWidth * sizeof(u32bits);
            for(x=0; x<(s32bits)dwWidth; x++)
            {
                switch(dwDepth)
                {
                    case    24:
                        b = *s; s++;
                        g = *s; s++;
                        r = *s; s++;
                        a = 0xFF;
                        break;

                    case    32:
                        b = *s; s++;
                        g = *s; s++;
                        r = *s; s++;
                        a = *s; s++;
                        break;
                }

                *pByte = r;
                pByte++;
                *pByte = g;
                pByte++;
                *pByte = b;
                pByte++;
                *pByte = a;
                pByte++;
            }
            //psrc -= dwWidth;
            //pByte = (u8bits *)psrc;
        }
    }
    else if(tgaImageType == 10)
    {
        for(y=dwHeight-1; y>=0; y--)
        {
            pByte = pbits + y * dwWidth * sizeof(u32bits);
            for(x=0; x<(s32bits)dwWidth; )
            {
                PacketInfo = *s; s++;
                PacketType = 0x80 & PacketInfo;
                PixelCount = (PacketInfo & 0x007F) + 1;

                if(PacketType) // run-length packet
                {
                    switch(dwDepth)
                    {
                        case    24:
                            b = *s; s++;
                            g = *s; s++;
                            r = *s; s++;
                            a = 0xFF;
                            break;

                        case    32:
                            b = *s; s++;
                            g = *s; s++;
                            r = *s; s++;
                            a = *s; s++;
                            break;
                    }

                    for(i=0; i<PixelCount; i++)
                    {
                        *pByte = r;
                        pByte++;
                        *pByte = g;
                        pByte++;
                        *pByte = b;
                        pByte++;
                        *pByte = a;
                        pByte++;
                        x++;
                        if(x == dwWidth) // run spans across rows
                        {
                            x = 0;
                            if(y > 0) y--;
                            else      goto breakOut;
                            pByte = pbits + y * dwWidth * sizeof(u32bits);
                        }
                    }
                }
                else // non run-length packet
                {
                    for(i=0; i<PixelCount; i++)
                    {
                        switch(dwDepth)
                        {
                            case    24:
                                b = *s; s++;
                                g = *s; s++;
                                r = *s; s++;
                                a = 0xFF;
                                break;

                            case    32:
                                b = *s; s++;
                                g = *s; s++;
                                r = *s; s++;
                                a = *s; s++;
                                break;
                        }
                        *pByte = r;
                        pByte++;
                        *pByte = g;
                        pByte++;
                        *pByte = b;
                        pByte++;
                        *pByte = a;
                        pByte++;
                        x++;
                        if(x == dwWidth) // pixel packet run spans across rows
                        {
                            x = 0;
                            if(y > 0) y--;
                            else      goto breakOut;
                            pByte = pbits + y * dwWidth * sizeof(u32bits);
                        }
                    }
                }
            }
            breakOut:;
        }
    }

    if(tgaImageDescriptor == 0 || tgaImageDescriptor == 0x08) /* 是否要上下顛倒 */
    {
        pline = (u32bits *)malloc(dwWidth * sizeof(u32bits));
        if(pline == null)
        {
            free(pbits);
            return null;
        }
        psrc = (u32bits *)pbits;
        pdst = (u32bits *)pbits + (dwWidth * (dwHeight - 1));
        for(y=0; y<(s32bits)(dwHeight>>1); y++)
        {
            memcpy(pline, psrc, dwWidth * sizeof(u32bits));
            memcpy(psrc, pdst, dwWidth * sizeof(u32bits));
            memcpy(pdst, pline, dwWidth * sizeof(u32bits));
            psrc += dwWidth;
            pdst -= dwWidth;
        }
        free((void*)pline);
    }
    
    
    /* 配置貼圖 */
    texture = (TEXTURE2D *)malloc(sizeof(TEXTURE2D));
    if(texture == null)
    {
		free(pbits);
        return null;
    }

	texture->iw           = iw;
	texture->ih           = ih;
	texture->tw           = tw;
	texture->th           = th;
	texture->tl           = 0.0f;
	texture->tu           = 0.0f;
	texture->tr           = u;
	texture->td           = v;
	//texture->texID        = texID;
	texture->size  		  = size;

    glGenTextures(1, &texture->texID);
    glBindTexture(GL_TEXTURE_2D, texture->texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(linear)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, pbits);
    
    /* 釋放暫存資料 */
    free(pbits);

    return texture;
}
//-----------------------------------------------------------------------------
static  TEXTURE2D 	*(*read_texture_memory_file[])(u8bits *file_buf, u32bits buf_size, bool linear)=
{
	gl_texture2d_read_dds_memory_file,
	gl_texture2d_read_tga_memory_file,
};
//-----------------------------------------------------------------------------
TEXTURE2D	*gl_texture2d_load_memory_file(u8bits *file_buf, u32bits buf_size, bool linear)
{
    u32bits     i, total;
    TEXTURE2D	*texture;

	non_power_of_two = true;
	
    texture = null;

    total = sizeof(read_texture_memory_file)/sizeof(read_texture_memory_file[0]);
    for(i=0; i<total; i++)
    {
        texture = (*read_texture_memory_file[i])(file_buf, buf_size, linear);
        if(texture)
        {
            break;
        }
    }

    return texture;  
}
//-----------------------------------------------------------------------------
TEXTURE2D	*gl_texture2d_load_file(char *file_name, bool linear)
{
    FILE        *fp;
	u8bits		*file_buf;
	u32bits		buf_size;
    TEXTURE2D	*texture;
		
#if defined(WIN32)
    if(fopen_s(&fp, (char *)file_name, "rb") != 0) return null;
#else
	if((fp = fopen(file_name, "rb")) == null) return null;
#endif

	fseek(fp, 0, SEEK_END);
	buf_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	file_buf = malloc(buf_size);
	if(file_buf == null) 
	{ 
		fclose(fp); 
		return null; 
	}
	
	if(fread(file_buf, sizeof(u8bits), buf_size, fp) != buf_size) 
	{ 
		free(file_buf); 
		fclose(fp); 
		return null; 
	}
	
	texture = gl_texture2d_load_memory_file(file_buf, buf_size, linear);
	
	free(file_buf);
	
    fclose(fp);
	
    return texture;
}
//-----------------------------------------------------------------------------
void    gl_texture2d_free(TEXTURE2D *texture)
{
    if(texture)
    {
        if(glIsTexture(texture->texID))
        {
			glDeleteTextures(1, (const GLuint*)&texture->texID);
        }
        
        free(texture);
    }
}
//-----------------------------------------------------------------------------
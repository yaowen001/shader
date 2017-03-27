//---------------------------------------------------------------------------
#include    <stdio.h>
#include    <SDL.h>

#include    "app_window.h"
//---------------------------------------------------------------------------
static  unsigned int m_ScreenWidth, m_ScreenHeight;
static  SDL_Window  *m_pMainWindow;
//-----------------------------------------------------------------------------
#define DEF_DEFAULT_WIDTH	(640)
#define DEF_DEFAULT_HEIGHT 	(480)
//-----------------------------------------------------------------------------
void	app_window_init(void)
{   
	m_ScreenWidth = DEF_DEFAULT_WIDTH;
    m_ScreenHeight = DEF_DEFAULT_HEIGHT;
	
	m_pMainWindow = null;
}
//-----------------------------------------------------------------------------
void	app_window_end(void)
{
}	
//-----------------------------------------------------------------------------
bool    app_window_create(char *title, unsigned int w, unsigned int h)
{
    unsigned int flags;
	
    flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;	
	
	// 分享 context
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1); 
    
	// 顏色格式
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	// 深度格式
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	// 雙緩衝
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);    

    // 建立視窗
    m_pMainWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
    if(m_pMainWindow == null)
    {
		system("pause");

        return false;
    }
	
    m_ScreenWidth = w;
    m_ScreenHeight = h;
    
    return true;
}
//-----------------------------------------------------------------------------
void    app_window_destroy(void)
{
	// 銷毀視窗
    if(m_pMainWindow)
    {
        SDL_DestroyWindow(m_pMainWindow);
		m_pMainWindow = null;
    }    
}
//-----------------------------------------------------------------------------
SDL_Window *app_window_get(void)
{
	if(m_pMainWindow) return m_pMainWindow;
	
	return null;
}
//-----------------------------------------------------------------------------
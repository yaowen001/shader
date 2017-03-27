//---------------------------------------------------------------------------
#include    <stdio.h>
#include    <SDL.h>

#include    "app_context.h"
//---------------------------------------------------------------------------
static  SDL_Window  	*m_pContextWindow;
static  SDL_GLContext   m_pRenderContext;
static  SDL_GLContext   m_pLoadContext;
static 	char 			*m_pCotextErrorString;
//-----------------------------------------------------------------------------
void    app_context_init(void)
{
	m_pContextWindow = null;
	
    m_pRenderContext = null;
    m_pLoadContext = null;
	
	m_pCotextErrorString = null;
}
//-----------------------------------------------------------------------------
void    app_context_end(void)
{
}
//-----------------------------------------------------------------------------
bool 	app_context_create(SDL_Window *pWindow)
{
    m_pRenderContext = SDL_GL_CreateContext(pWindow);
    if(m_pRenderContext == null)
	{
		m_pCotextErrorString = (char *)"SDL_GL_CreateContext fail. (render context)\n";
		return false;
	}

    m_pLoadContext = SDL_GL_CreateContext(pWindow);
	if(m_pLoadContext == null)
	{
		SDL_GL_DeleteContext(m_pRenderContext);
        m_pRenderContext = null;
		
        m_pCotextErrorString = (char *)"SDL_GL_CreateContext fail. (load context)\n";
	}
	
	m_pContextWindow = pWindow;
	
	return true;
}
//-----------------------------------------------------------------------------
void    app_context_destroy(void)
{
    if(m_pLoadContext)
    {
        SDL_GL_DeleteContext(m_pLoadContext);
        m_pLoadContext = null;
    }

    if(m_pRenderContext)
    {
        SDL_GL_DeleteContext(m_pRenderContext);
        m_pRenderContext = null;
    }
	
	m_pContextWindow = null;
}
//-----------------------------------------------------------------------------
bool 	app_context_bind_render_context(bool n)
{
    if(n)
    {
        if(SDL_GL_MakeCurrent(m_pContextWindow, m_pRenderContext) != 0)
        {
            m_pCotextErrorString = (char *)"SDL_GL_MakeCurrent fail. (bind render context)\n";
			return false;
        }                
    }
    else
    {
        if(SDL_GL_MakeCurrent(m_pContextWindow, NULL) != 0)
        {
            m_pCotextErrorString = (char *)"SDL_GL_MakeCurrent fail. (unbind render context)\n";
			return false;
        }        
    }

    return true;
}
//-----------------------------------------------------------------------------
bool 	app_context_bind_load_context(bool n)
{
    if(n)
    {
        if(SDL_GL_MakeCurrent(m_pContextWindow, m_pLoadContext) != 0)
        {
            m_pCotextErrorString = (char *)"SDL_GL_MakeCurrent fail. (bind load context)\n";
			return false;            
        }                
    }
    else
    {
        if(SDL_GL_MakeCurrent(m_pContextWindow, NULL) != 0)
        {
            m_pCotextErrorString = (char *)"SDL_GL_MakeCurrent fail. (unbind load context)\n"; 
			return false;
        }        
    }

    return true;
}
//-----------------------------------------------------------------------------
void    app_context_vsync_wait_set(unsigned int n)
{
    SDL_GL_SetSwapInterval(n);    
}
//-----------------------------------------------------------------------------
void    app_context_swap_buffer(void)
{
    if(m_pContextWindow == null) return;

 	SDL_GL_SwapWindow(m_pContextWindow); // 更新到瑩幕
}
//-----------------------------------------------------------------------------
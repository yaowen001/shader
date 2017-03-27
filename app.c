//---------------------------------------------------------------------------
#include    <stdio.h>
#include    <SDL.h>

#include    "app.h"
#include    "gl_texture2d.h"
#include    "gl_shader.h"
//---------------------------------------------------------------------------
static  bool 	m_bAppQuit; // 程式結束
static  unsigned int m_ScreenWidth, m_ScreenHeight;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static	double  app_fps;
static	double  app_frame_time;
static	double  app_delay_time;
static	double  app_start_ticks;
static	double	app_end_ticks;
//-----------------------------------------------------------------------------
static 	void	app_sleep_init(void)
{
	app_fps = 60;
	app_frame_time = 1000 / app_fps;
	app_delay_time = app_frame_time;
	app_start_ticks = -1;
}
//-----------------------------------------------------------------------------
static 	void	app_sleep(void)
{
    if(app_start_ticks > 0) 
    {
        app_end_ticks = SDL_GetTicks();
        app_delay_time += (app_frame_time - (app_end_ticks - app_start_ticks));
        app_start_ticks = app_end_ticks;
        if(app_delay_time < 0)
        {
            app_delay_time = app_frame_time;
        }
    }
    else 
    {
        app_start_ticks = SDL_GetTicks();
    }
    SDL_Delay((unsigned int)app_delay_time);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static  void    app_window_events(SDL_Event *event)
{
    if(event->type == SDL_WINDOWEVENT)
    {
        switch(event->window.event)
        {
        #if 0
            case    SDL_WINDOWEVENT_NONE:
            case    SDL_WINDOWEVENT_SHOWN:
            case    SDL_WINDOWEVENT_HIDDEN:
            case    SDL_WINDOWEVENT_EXPOSED:
            case    SDL_WINDOWEVENT_MOVED:
                break;

            case    SDL_WINDOWEVENT_RESIZED:
                printf("resize->%dx%d\n", event->window.data1, event->window.data2);
                break;

            case    SDL_WINDOWEVENT_SIZE_CHANGED:
                break;
                
            case    SDL_WINDOWEVENT_MINIMIZED:
                break;
            case    SDL_WINDOWEVENT_MAXIMIZED:
                break;
            case    SDL_WINDOWEVENT_RESTORED:
                break;
                
            case    SDL_WINDOWEVENT_ENTER:
            case    SDL_WINDOWEVENT_LEAVE:
            case    SDL_WINDOWEVENT_FOCUS_GAINED:
            case    SDL_WINDOWEVENT_FOCUS_LOST:
        #endif
            case    SDL_WINDOWEVENT_CLOSE:
                m_bAppQuit = true;
                break;
        }
    }
}
//-----------------------------------------------------------------------------
#define MAX_KEYBOARD_RECORD     (256)
static  bool    m_bKeyboardState[MAX_KEYBOARD_RECORD];
//-----------------------------------------------------------------------------
void    app_keyboard_init(void)
{
    unsigned int 	i;

    for(i=0; i<MAX_KEYBOARD_RECORD; i++)
    {
        m_bKeyboardState[i] = false;
    }
}
//-----------------------------------------------------------------------------
void    app_keyboard_events(SDL_Event *event)
{
    switch(event->type)
    {
        case    SDL_KEYDOWN:
            m_bKeyboardState[event->key.keysym.sym & 0xFF] = true;
            break;

        case    SDL_KEYUP:
            m_bKeyboardState[event->key.keysym.sym & 0xFF] = false;
            break;
    }
}
//-----------------------------------------------------------------------------
bool    app_keyboard_pressed(unsigned int keyid)
{
    return m_bKeyboardState[keyid];
}
//-----------------------------------------------------------------------------
static  void    app_event(void)
{
    SDL_Event   event;
    
    while(SDL_PollEvent(&event)) // 事件
    {
        app_window_events(&event); // window 事件
        app_keyboard_events(&event); // keyboard 事件

        if(event.type == SDL_QUIT)
        {
            m_bAppQuit = true;
        }
    }
}
//-----------------------------------------------------------------------------
static 	RENDER_TEXTURE	*m_pRenderTexture;

static 	TEXTURE2D 		*texture;

static 	TTF_FONT		*ttf_font;

static  bool	app_init(void)
{	
	m_bAppQuit = false;
	
	m_ScreenWidth = 640;
	m_ScreenHeight = 480;
	
	// SDL 初始化
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL_Init() fail.\n");
		system("pause");
		return false;	
	}	
    printf("SDL_Init() done.\n");
		
	app_sleep_init(); // 休眠初始化
	app_window_init(); // 視窗相關變數初始化
	app_context_init(); // context 相關變數初始化
    app_keyboard_init(); // keyboard 變數初始化

    // 建立視窗
    if(app_window_create((char *)"SDL2 test", m_ScreenWidth, m_ScreenHeight) == false)
    {
		SDL_Quit(); // SDL 結束

		printf("app_window_create() fail.\n");
		system("pause");
		return false;
	}
    printf("app_window_create() done.\n");
		
    // 建立 context
    if(app_context_create(app_window_get()) == false)
	{
		app_window_destroy(); // 銷毀視窗
		SDL_Quit(); // SDL 結束
		
		printf("app_context_create() fail.\n");
		system("pause");
		return false;
	}
    printf("app_context_create() done.\n");
	
    if(app_context_bind_render_context(1) == false) // 綁定 render context
	{
		app_context_destroy(); // 銷毀 context
		app_window_destroy(); // 銷毀視窗
		SDL_Quit(); // SDL 結束
		
		printf("app_context_bind_render_context(1) fail.\n");
		system("pause");
		return false;
	}	
    printf("app_context_bind_render_context(1) done.\n");
	
	app_context_vsync_wait_set(0); // 設定要等垂直返馳

	// shader 初始化
	if(gl_shader_init(m_ScreenWidth, m_ScreenHeight) == false)
	{
		app_context_bind_render_context(0); // 取消綁定 render context        
		app_context_destroy(); // 銷毀 context
		app_window_destroy(); // 銷毀視窗
		SDL_Quit(); // SDL 結束
		
		printf("gl_shader_init() fail.\n");
		system("pause");
		return false;
	}	
    printf("gl_shader_init() done.\n");
	
	m_pRenderTexture = (RENDER_TEXTURE *)gl_shader2d_fbo_alloc(m_ScreenWidth, m_ScreenHeight);

	texture = gl_texture2d_load_file("sdl.tga", true);

	ttf_font = gl_shader2d_ttf_open_file("NotoSansCJKtc-Thin.otf", 24);
	
	return true;
}
//-----------------------------------------------------------------------------
static  void    app_end(void)
{
	if(texture) { gl_texture2d_free(texture); texture = null; }
	
	if(m_pRenderTexture) { gl_shader2d_fbo_free(m_pRenderTexture); m_pRenderTexture = null; }
	
	if(ttf_font) { gl_shader2d_ttf_close_file(ttf_font); ttf_font = null; }
	
	gl_shader_end(); // shader 結束
	printf("gl_shader_end() done.\n");
	
	app_context_bind_render_context(0); // 取消綁定 render context        
	printf("app_context_bind_render_context(0) done.\n");
	
    app_context_destroy(); // 銷毀 context
	printf("app_context_destroy() done.\n");
		
    app_window_destroy(); // 銷毀視窗
    printf("app_window_destroy() done.\n");
    
	SDL_Quit(); // SDL 結束
    printf("SDL_Quit() done.\n");

	printf("app exit\n");
}
//-----------------------------------------------------------------------------
void    app_set_quit(void)
{
    m_bAppQuit = true;
}
//-----------------------------------------------------------------------------
void    app_present(void)
{
	s32bits 	x, y;
	
	#if 0
	double start_ticks, end_ticks;

	start_ticks = SDL_GetTicks();
	glClear(GL_COLOR_BUFFER_BIT); // 清畫面
	end_ticks = SDL_GetTicks();
	//printf("glClear:%f\n", end_ticks - start_ticks);
	#endif
	
	x = 16;
	y = 16;
	gl_shader2d_clear();
	gl_shader2d_draw_begin();
			
			if(m_pRenderTexture)
			{	
				gl_shader2d_fbo_draw_begin(m_pRenderTexture, 0x00, 0x00, 0x00, 0xFF);
			}
			
			gl_shader2d_draw_pixel(x, y, 0xFF, 0x00, 0x00, 0xFF);
			y += 16;
			gl_shader2d_draw_line(x, y, 100, y, 0xFF, 0x00, 0x00, 0xFF);
			y += 16;
			gl_shader2d_draw_rect(x, y, x + 32, y + 32, 0xFF, 0x00, 0x00, 0xFF);
			y += 48;
			gl_shader2d_draw_rect_fill(x, y, x + 32, y + 32, 0xFF, 0x00, 0x00, 0xFF);
			y += 64;
			gl_shader2d_draw_circle(x + 16, y, 20, 0x00, 0xFF, 0x00, 0xFF);
			y += 48;
			gl_shader2d_draw_circle_fill(x + 16, y, 20, 0x00, 0xFF, 0x00, 0xFF);
			y += 48;
			gl_shader2d_draw_ellipse(x + 16, y, 10, 20, 0x00, 0x00, 0xFF, 0xFF);
			y += 48;
			gl_shader2d_draw_ellipse_fill(x + 16, y, 10, 20, 0x00, 0x00, 0xFF, 0xFF);
			
			if(texture)
			{
				y += 32;
				gl_shader2d_draw_texture(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR,
										 x, y, texture->iw, texture->ih,
										 0xFF, 0xFF, 0xFF, 0xFF,
										 false, false,
										 0, 
										 texture->texID, texture->tl, texture->tu, texture->tr, texture->td);
			}		

			if(ttf_font)
			{
				gl_shader2d_ttf_draw_glyph(ttf_font, 100, 0, 0xFF, 0xFF, 0x00, 0xFF, (wchar_t *)L"TTF測試");			
			}
			
			if(m_pRenderTexture)
			{	
				gl_shader2d_printf(16, 0, 0xFF, 0xFF, 0xFF, 0xFF, "FBO");
				gl_shader2d_fbo_draw_end(m_pRenderTexture);
				gl_shader2d_fbo_draw_texture(m_pRenderTexture, DEF_USE_PROGRAM_TYPE_DEFAULT, 0, 0, 640, 480, 0xFF, 0xFF, 0xFF, 0xFF, false, false, 0);
			}
			
	gl_shader2d_draw_end();;
	app_context_swap_buffer(); // 更新到瑩幕
}
//-----------------------------------------------------------------------------
bool    app_run(void)
{
	if(app_init() == false) // app 初始化
    {
		return false;
	}
	
	while(m_bAppQuit == false)
	{
		app_event(); // 事件
		
		
		app_present(); // 顯示畫面
		
		
		app_sleep(); // 休眠
	}
	
	app_end(); // app 結束
	
	system("pause");
	
    return true;
}
//-----------------------------------------------------------------------------
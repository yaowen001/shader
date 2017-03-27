//-----------------------------------------------------------------------------
#ifndef __APP_WINDOW_H
#define __APP_WINDOW_H

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
DEF_EXTERN	void	app_window_init(void); // window 相關變數初始化
DEF_EXTERN	void	app_window_end(void); // window  結束
//-----------------------------------------------------------------------------
DEF_EXTERN 	bool    app_window_create(char *title, unsigned int w, unsigned int h); // 建立 window
DEF_EXTERN 	void    app_window_destroy(void); // 銷毀 window
//-----------------------------------------------------------------------------
DEF_EXTERN 	SDL_Window *app_window_get(void); // 取得 SDL_window 指標
//-----------------------------------------------------------------------------

#endif // __APP_WINDOW_H
//-----------------------------------------------------------------------------

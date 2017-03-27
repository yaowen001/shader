//-----------------------------------------------------------------------------
#ifndef __APP_CONTEXT_H
#define __APP_CONTEXT_H

//-----------------------------------------------------------------------------
#include    <SDL.h>
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
DEF_EXTERN 	void    app_context_init(void); // context 相關變數初始化
DEF_EXTERN 	void    app_context_end(void); // context 結束
//-----------------------------------------------------------------------------
DEF_EXTERN 	bool 	app_context_create(SDL_Window *pWindow); // 建立視窗的 context
DEF_EXTERN 	void    app_context_destroy(void); // 銷毀視窗的 context
//-----------------------------------------------------------------------------
DEF_EXTERN 	bool 	app_context_bind_render_context(bool n); // 綁定繪製 context
DEF_EXTERN 	bool 	app_context_bind_load_context(bool n); // 綁定載入 context
DEF_EXTERN 	void    app_context_vsync_wait_set(unsigned int n); // 設定是否等垂直同步
DEF_EXTERN 	void 	app_context_swap_buffer(void); // 顯示完成的畫面
//-----------------------------------------------------------------------------

#endif // __APP_CONTEXT_H
//-----------------------------------------------------------------------------

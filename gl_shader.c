//-----------------------------------------------------------------------------
#include    <stdio.h>
#include    <stdlib.h>

#include    "gl_shader.h"
//-----------------------------------------------------------------------------
static 	bool	shader_init_ok;
//-----------------------------------------------------------------------------
bool 	gl_shader_init(u32bits width, u32bits height)
{
	shader_init_ok = false;
	
	// shader2d 初始化
	if(gl_shader2d_init(width, height) == false)
	{
		return false;
	}
	
	// shader2d ttf 初始化
	if(gl_shader2d_ttf_init() == false)
	{
		gl_shader2d_end(); // shader2d 結束
		return false;
	}
	
	shader_init_ok = true;
	
	return true;
}
//-----------------------------------------------------------------------------
void 	gl_shader_end(void)
{
	if(shader_init_ok)
	{
		gl_shader2d_ttf_end(); // shader2d ttf 結束
		
		gl_shader2d_end(); // shader2d 結束
		
		shader_init_ok = false;
	}
}
//-----------------------------------------------------------------------------
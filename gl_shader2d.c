//-----------------------------------------------------------------------------
#include    <stdio.h>
#include    <stdlib.h>
#include    <memory.h>
#include    <math.h>
#include    <GL/glew.h>

#include    "gl_shader2d.h"
//-----------------------------------------------------------------------------
static  u32bits m_ViewWidth, m_ViewHeight;
static  bool    non_power_of_two;
//-----------------------------------------------------------------------------
#define	DEF_PI	(3.145926535f)

static 	float   g_fByteToFloatTable[256];
static 	float   g_fSinTable[361];
static 	float   g_fCosTable[361];
static 	float   g_fTanTable[361];

static 	void 	gl_shader2d_lookup_table_init(void)
{
	u32bits	i;
	
	// byte(0 ~ 255) to float(0.000000f ~ 1.000000f) 
    for(i=0; i<256; i++)
    {
        g_fByteToFloatTable[i] = (float)((float)i / 256.0f);
    }
	
	for(i=0; i<=360; i++)
	{
        g_fSinTable[i] = sinf(((float)i * DEF_PI) / 180.0f);
        g_fCosTable[i] = cosf(((float)i * DEF_PI) / 180.0f);		
        g_fTanTable[i] = tanf(((float)i * DEF_PI) / 180.0f);		
	}
}
//-----------------------------------------------------------------------------
#define DEF_NEAR_PLANE_2D   ( 32768.0f)
#define DEF_FAR_PLANE_2D    (-32767.0f)

static  float   m_Projection[16];
static  float   m_Modelview[16];

static  float   m_ProjectionModelview[16];
static  float   m_ProjectionModelviewFlipY[16];

static  bool 	m_bFlipY, m_bPrevFlipY;
//-----------------------------------------------------------------------------
static  void    gl_shader2d_ortho_init(float *tmp_matrix, float left, float right, float bottom, float top, float zNear, float zFar)
{
    float r_l, t_b, f_n;
    float tx, ty, tz;
    float x, y, z;
    float s, c;
    s32bits angle;
    s32bits offset_j, offset_k;
    s32bits i, j, k;
    s32bits array_offset_tab[]={ 0, 4, 8, 12};
    
    
    r_l = right - left;
    t_b = top - bottom;
    f_n = zFar - zNear;
    
    tx = - (right + left) / (right - left);
    ty = - (top + bottom) / (top - bottom);
    tz = - (zFar + zNear) / (zFar - zNear);
    
    x = 2.0f / r_l;
    y = 2.0f / t_b;
    z = 2.0f / f_n;
    
    //
     m_Projection[ 0] =   x; m_Projection[ 1] = 0.0; m_Projection[ 2] = 0.0; m_Projection[ 3] = 0.0;
     m_Projection[ 4] = 0.0; m_Projection[ 5] =   y; m_Projection[ 6] = 0.0; m_Projection[ 7] = 0.0;
     m_Projection[ 8] = 0.0; m_Projection[ 9] = 0.0; m_Projection[10] =   z; m_Projection[11] = 0.0;
     m_Projection[12] =  tx; m_Projection[13] =  ty; m_Projection[14] =  tz; m_Projection[15] = 1.0;
    
    m_Projection[ 0] =   x;
    m_Projection[ 5] =   y;
    m_Projection[10] =   z;
    m_Projection[12] =  tx; m_Projection[13] =  ty; m_Projection[14] =  tz;
    
    
    angle = 0;
    s = g_fSinTable[angle];
    c = g_fCosTable[angle];
    
    //
     m_Modelview[ 0] =   c; m_Modelview[ 1] =   s; m_Modelview[ 2] = 0.0; m_Modelview[ 3] = 0.0;
     m_Modelview[ 4] =  -s; m_Modelview[ 5] =   c; m_Modelview[ 6] = 0.0; m_Modelview[ 7] = 0.0;
     m_Modelview[ 8] = 0.0; m_Modelview[ 9] = 0.0; m_Modelview[10] = 1.0; m_Modelview[11] = 0.0;
     m_Modelview[12] = 0.0; m_Modelview[13] = 0.0; m_Modelview[14] = 0.0; m_Modelview[15] = 1.0;
    
    m_Modelview[ 0] =   c; m_Modelview[ 1] =   s;
    m_Modelview[ 4] =  -s; m_Modelview[ 5] =   c;
    
    if(tmp_matrix)
    {
        for(i=0; i<4; i++)
        {
            for(j=0; j<4; j++)
            {
                offset_j = array_offset_tab[j];
                tmp_matrix[offset_j + i] = 0.0f;
            
                for(k=0; k<4; k++)
                {
                    offset_k = array_offset_tab[k];
                    tmp_matrix[offset_j + i] += (m_Projection[offset_k + i] * m_Modelview[offset_j + k]);
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------
static  void    gl_shader2d_ortho_matrix_init(u32bits w, u32bits h)
{
    u32bits i;
    
    for(i=0; i<16; i++)
    {
        m_Modelview[i] = 0.0f;
    }
    m_Modelview[10] = 1.0f;
    m_Modelview[15] = 1.0f;
    
    for(i=0; i<16; i++)
    {
        m_Projection[i] = 0.0f;
    }
    m_Projection[15] = 1.0f;
    
    gl_shader2d_ortho_init(m_ProjectionModelview, 0.0f, (float)w, (float)h, 0.0f, DEF_NEAR_PLANE_2D, DEF_FAR_PLANE_2D);
    gl_shader2d_ortho_init(m_ProjectionModelviewFlipY, 0.0f, (float)w, 0.0f, (float)h, DEF_NEAR_PLANE_2D, DEF_FAR_PLANE_2D);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static  GLuint  gl_shader_build_complie(const char *source_name, u32bits source_type)
{
    GLuint  shader;
    GLint   success;
    
    int infologLength = 0;
    int infologWritten = 0;
    char *infoLog;

    shader = glCreateShader(source_type);
    
    glShaderSource(shader, 1, &source_name, null);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(success == GL_FALSE)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
        if(infologLength > 0)
        {
            infoLog = (char *)malloc(infologLength);
            glGetShaderInfoLog(shader, infologLength, &infologWritten, infoLog);
			printf("shader compiler error: %s\n", infoLog);
            free(infoLog);
        }

        return 0;
    }

    return shader;
}
//-----------------------------------------------------------------------------
static  GLuint  gl_shader_build_link(GLuint idVertexShader, GLuint idFragmentShader)
{
    GLuint  program;
    GLint   success;

    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
    
    program = glCreateProgram();
    
    glAttachShader(program, idVertexShader);
    glAttachShader(program, idFragmentShader);
    
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(success == GL_FALSE)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);
        if(infologLength > 0)
        {
            infoLog = (char *)malloc(infologLength);
            glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog);
			printf("shader linker error: %s\n", infoLog);			
            free(infoLog);
        }

        return 0;
    }
    
    return program;
}
//-----------------------------------------------------------------------------
static  char    *code_error_string;
static	bool 	gl_shader_build_alloc(char *pVertexSource, char *pFragmentSource, GLuint *pVertexShaderID, GLuint *pFragmentShaderID, GLuint *pShaderProgramID)
{
    GLuint  idVertexShader;
    GLuint  idFragmentShader;
    GLuint  idShaderProgram;

    // compiler vertex shader code
    idVertexShader = gl_shader_build_complie(pVertexSource, GL_VERTEX_SHADER);
    if(!idVertexShader)
    {
        code_error_string = (char *)"shader complier error : (GL_VERTEX_SHADER).\n";
        return false;
    }

    // compiler fragment shader code
    idFragmentShader = gl_shader_build_complie(pFragmentSource, GL_FRAGMENT_SHADER);
    if(!idFragmentShader)
    {
        code_error_string = (char *)"shader complier error : (GL_FRAGMENT_SHADER).\n";
        return false;
    }
    
    // linker
    idShaderProgram = gl_shader_build_link(idVertexShader, idFragmentShader);
    if(!idShaderProgram)
    {
        code_error_string = (char *)"shader linker error.\n";
        return false;
    }

    *pVertexShaderID = idVertexShader;
    *pFragmentShaderID = idFragmentShader;
    *pShaderProgramID = idShaderProgram;

    return true;
}
//-----------------------------------------------------------------------------
static 	void    gl_shader_build_free(GLuint idProgram, GLuint idVertexShader, GLuint idFragmentShader)
{
    glDetachShader(idProgram, idVertexShader);
    glDetachShader(idProgram, idFragmentShader);
    
    glDeleteShader(idVertexShader);
    glDeleteShader(idFragmentShader);

    glDeleteProgram(idProgram);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static  const   char    *m_default_vertex_2d_source=
{
#if defined(__USE_OPENGL_ES2__)
    "precision highp float;"
#endif
    
    "uniform mat4 u_ProjectionModelview;"
    
    "attribute vec4 a_Position;"
    "attribute vec4 a_Color;"
    "attribute vec2 a_TexCoord;"
    
    "varying vec4 v_color;"
    "varying vec2 v_uv;"
    
    "void main()"
    "{"
        "v_color = a_Color;"
        "v_uv = a_TexCoord;"
    
        "gl_Position = u_ProjectionModelview * a_Position;"
    "}"
};
//-----------------------------------------------------------------------------
static  const   char    *m_default_fragment_2d_source=
{
#if defined(__USE_OPENGL_ES2__)
    "precision highp float;"
#endif
    
    "uniform sampler2D u_Texture;"
    "varying vec4 v_color;"
    "varying vec2 v_uv;"
    
    "void main()"
    "{"
        "gl_FragColor = texture2D(u_Texture, v_uv);"
    "}"
};
//-----------------------------------------------------------------------------
static  const   char    *m_default_vertex_fragment_2d_source=
{
#if defined(___USE_OPENGL_ES2__)
    "precision highp float;"
#endif
    
    "uniform sampler2D u_Texture;"
    "varying vec4 v_color;"
    "varying vec2 v_uv;"
    
    "void main()"
    "{"
        "gl_FragColor = texture2D(u_Texture, v_uv) * v_color;"
    "}"
};
//-----------------------------------------------------------------------------
static  GLuint  m_idDefault_VertexShader;
static  GLuint  m_idDefault_FragmentShader;
static  GLuint  m_idDefault_ShaderProgram;

static  GLuint  m_Default_TextureSlot;
static  GLuint  m_Default_ProjectionModelviewSlot;
static  GLuint  m_Default_PositionSlot;
static  GLuint  m_Default_ColorSlot;
static  GLuint  m_Default_TexCoordSlot;
//-----------------------------------------------------------------------------
static 	bool	gl_shader2d_program_default_init(void)
{
    if(gl_shader_build_alloc((char *)m_default_vertex_2d_source, (char *)m_default_fragment_2d_source,
                             (GLuint *)&m_idDefault_VertexShader, (GLuint *)&m_idDefault_FragmentShader,
                             (GLuint *)&m_idDefault_ShaderProgram) == false)
    {
        return false;
    }

    glUseProgram(m_idDefault_ShaderProgram);
    
    m_Default_TextureSlot = glGetUniformLocation(m_idDefault_ShaderProgram, "u_Texture");
	glUniform1i(m_Default_TextureSlot, 0);
    
    m_Default_ProjectionModelviewSlot = glGetUniformLocation(m_idDefault_ShaderProgram, "u_ProjectionModelview");
    glUniformMatrix4fv(m_Default_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
   
    m_Default_PositionSlot = glGetAttribLocation(m_idDefault_ShaderProgram, "a_Position");
    m_Default_ColorSlot = glGetAttribLocation(m_idDefault_ShaderProgram, "a_Color");
    m_Default_TexCoordSlot = glGetAttribLocation(m_idDefault_ShaderProgram, "a_TexCoord");

    return true;
}
//-----------------------------------------------------------------------------
static 	void    gl_shader2d_program_default_end(void)
{
    gl_shader_build_free(m_idDefault_ShaderProgram, m_idDefault_VertexShader, m_idDefault_FragmentShader);
}
//-----------------------------------------------------------------------------
static  GLuint  m_idDefaultVertexColor_VertexShader;
static  GLuint  m_idDefaultVertexColor_FragmentShader;
static  GLuint  m_idDefaultVertexColor_ShaderProgram;

static  GLuint  m_DefaultVertexColor_TextureSlot;
static  GLuint  m_DefaultVertexColor_ProjectionModelviewSlot;
static  GLuint  m_DefaultVertexColor_PositionSlot;
static  GLuint  m_DefaultVertexColor_ColorSlot;
static  GLuint  m_DefaultVertexColor_TexCoordSlot;
//-----------------------------------------------------------------------------
static  bool 	gl_shader2d_program_default_vertex_color_init(void)
{
    if(gl_shader_build_alloc((char *)m_default_vertex_2d_source, (char *)m_default_vertex_fragment_2d_source,
                             (GLuint *)&m_idDefaultVertexColor_VertexShader, (GLuint *)&m_idDefaultVertexColor_FragmentShader,
                             (GLuint *)&m_idDefaultVertexColor_ShaderProgram) == false)
    {
        return false;
    }

    glUseProgram(m_idDefaultVertexColor_ShaderProgram);
    
    m_DefaultVertexColor_TextureSlot = glGetUniformLocation(m_idDefaultVertexColor_ShaderProgram, "u_Texture");
    glUniform1i(m_DefaultVertexColor_TextureSlot, 0);
    
    m_DefaultVertexColor_ProjectionModelviewSlot = glGetUniformLocation(m_idDefaultVertexColor_ShaderProgram, "u_ProjectionModelview");
    glUniformMatrix4fv(m_DefaultVertexColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
    
    m_DefaultVertexColor_PositionSlot = glGetAttribLocation(m_idDefaultVertexColor_ShaderProgram, "a_Position");
    m_DefaultVertexColor_ColorSlot = glGetAttribLocation(m_idDefaultVertexColor_ShaderProgram, "a_Color");
    m_DefaultVertexColor_TexCoordSlot = glGetAttribLocation(m_idDefaultVertexColor_ShaderProgram, "a_TexCoord");
    
    return true;
}
//-----------------------------------------------------------------------------
static  void    gl_shader2d_program_default_vertex_color_end(void)
{
    gl_shader_build_free(m_idDefaultVertexColor_ShaderProgram, m_idDefaultVertexColor_VertexShader, m_idDefaultVertexColor_FragmentShader);
}
//-----------------------------------------------------------------------------
static  const   char    *m_assign_color_fragment_2d_source=
{
#if defined(__USE_OPENGL_ES2__)
    "precision highp float;"
#endif
    
    "uniform sampler2D u_Texture;"
    "varying vec4 v_color;"
    "varying vec2 v_uv;"
    
    "uniform vec4 u_AssignColor;"
    
    "void main()"
    "{"
        "vec4 curr_FragColor = texture2D(u_Texture, v_uv) * v_color;"
        "gl_FragColor = vec4(u_AssignColor.r, u_AssignColor.g, u_AssignColor.b, curr_FragColor.a);"
    "}"
};
//-----------------------------------------------------------------------------
static  GLuint  m_idAssignColor_VertexShader;
static  GLuint  m_idAssignColor_FragmentShader;
static  GLuint  m_idAssignColor_ShaderProgram;

static  GLuint  m_AssignColor_TextureSlot;
static  GLuint  m_AssignColor_ProjectionModelviewSlot;
static  GLuint  m_AssignColor_PositionSlot;
static  GLuint  m_AssignColor_ColorSlot;
static  GLuint  m_AssignColor_TexCoordSlot;
static  GLuint  m_AssignColor_AssignColorSlot;

static  u8bits  m_AssignR, m_AssignG, m_AssignB;
//-----------------------------------------------------------------------------
static  bool 	gl_shader2d_program_assign_color_init(void)
{
    if(gl_shader_build_alloc((char *)m_default_vertex_2d_source, (char *)m_assign_color_fragment_2d_source, 
                             (GLuint *)&m_idAssignColor_VertexShader, (GLuint *)&m_idAssignColor_FragmentShader,
                             (GLuint *)&m_idAssignColor_ShaderProgram) == false)
    {
        return false;
    }

    glUseProgram(m_idAssignColor_ShaderProgram);
    
    m_AssignColor_TextureSlot = glGetUniformLocation(m_idAssignColor_ShaderProgram, "u_Texture");
    glUniform1i(m_AssignColor_TextureSlot, 0);
    
    m_AssignColor_ProjectionModelviewSlot = glGetUniformLocation(m_idAssignColor_ShaderProgram, "u_ProjectionModelview");
    glUniformMatrix4fv(m_AssignColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
    
    m_AssignColor_AssignColorSlot = glGetUniformLocation(m_idAssignColor_ShaderProgram, "u_AssignColor");
    
    m_AssignColor_PositionSlot = glGetAttribLocation(m_idAssignColor_ShaderProgram, "a_Position");
    m_AssignColor_ColorSlot = glGetAttribLocation(m_idAssignColor_ShaderProgram, "a_Color");
    m_AssignColor_TexCoordSlot = glGetAttribLocation(m_idAssignColor_ShaderProgram, "a_TexCoord");
    
    m_AssignR = 0;
    m_AssignG = 0;
    m_AssignB = 0;
    
    return true;
}
//-----------------------------------------------------------------------------
static void    gl_shader2d_program_assign_color_end(void)
{
    gl_shader_build_free(m_idAssignColor_ShaderProgram, m_idAssignColor_VertexShader, m_idAssignColor_FragmentShader);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_assign_color_set(u8bits r, u8bits g, u8bits b)
{
    m_AssignR = r;
    m_AssignG = g;
    m_AssignB = b;
}
//-----------------------------------------------------------------------------
static  const   char    *m_negative_color_fragment_2d_source=
{
#if defined(__USE_OPENGL_ES2__)
    "precision highp float;"
#endif
    
    "uniform sampler2D u_Texture;"
    "varying vec4 v_color;"
    "varying vec2 v_uv;"
    
    "void main()"
    "{"
        "vec4 curr_FragColor = texture2D(u_Texture, v_uv) * v_color;"
        "gl_FragColor = vec4(1.0 - curr_FragColor.r, 1.0 - curr_FragColor.g, 1.0 - curr_FragColor.b, curr_FragColor.a);"
    "}"
};
//-----------------------------------------------------------------------------
static  GLuint  m_idNegativeColor_VertexShader;
static  GLuint  m_idNegativeColor_FragmentShader;
static  GLuint  m_idNegativeColor_ShaderProgram;

static  GLuint  m_NegativeColor_TextureSlot;
static  GLuint  m_NegativeColor_ProjectionModelviewSlot;
static  GLuint  m_NegativeColor_PositionSlot;
static  GLuint  m_NegativeColor_ColorSlot;
static  GLuint  m_NegativeColor_TexCoordSlot;

static  float   m_MaskR, m_MaskG, m_MaskB, m_MaskA;
//-----------------------------------------------------------------------------
static  bool 	gl_shader2d_program_negative_color_init(void)
{
    if(gl_shader_build_alloc((char *)m_default_vertex_2d_source, (char *)m_negative_color_fragment_2d_source, 
                                         (GLuint *)&m_idNegativeColor_VertexShader, (GLuint *)&m_idNegativeColor_FragmentShader, 
                                         (GLuint *)&m_idNegativeColor_ShaderProgram) == false)
    {
        return false;
    }

    glUseProgram(m_idNegativeColor_ShaderProgram);
    
    m_NegativeColor_TextureSlot = glGetUniformLocation(m_idNegativeColor_ShaderProgram, "u_Texture");
    glUniform1i(m_NegativeColor_TextureSlot, 0);
    
    m_NegativeColor_ProjectionModelviewSlot = glGetUniformLocation(m_idNegativeColor_ShaderProgram, "u_ProjectionModelview");
    glUniformMatrix4fv(m_NegativeColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
    
    m_NegativeColor_PositionSlot = glGetAttribLocation(m_idNegativeColor_ShaderProgram, "a_Position");
    m_NegativeColor_ColorSlot = glGetAttribLocation(m_idNegativeColor_ShaderProgram, "a_Color");
    m_NegativeColor_TexCoordSlot = glGetAttribLocation(m_idNegativeColor_ShaderProgram, "a_TexCoord");
    
    return true;
}
//-----------------------------------------------------------------------------
static void    gl_shader2d_program_negative_color_end(void)
{
    gl_shader_build_free(m_idNegativeColor_ShaderProgram, m_idNegativeColor_VertexShader, m_idNegativeColor_FragmentShader);
}
//-----------------------------------------------------------------------------
static  const   char    *m_gray_color_fragment_2d_source=
{
#if defined(__USE_OPENGL_ES2__)
    "precision highp float;"
#endif
    
    "uniform sampler2D u_Texture;"
    "varying vec4 v_color;"
    "varying vec2 v_uv;"
    
    "void main()"
    "{"
        "vec4 curr_FragColor = texture2D(u_Texture, v_uv) * v_color;"
        "float gray_color = curr_FragColor.r * 0.30 + curr_FragColor.g * 0.59 + curr_FragColor.b * 0.11;"
        "gl_FragColor = vec4(gray_color, gray_color, gray_color, curr_FragColor.a);"
    "}"
};
//-----------------------------------------------------------------------------
static  GLuint  m_idGrayColor_VertexShader;
static  GLuint  m_idGrayColor_FragmentShader;
static  GLuint  m_idGrayColor_ShaderProgram;

static  GLuint  m_GrayColor_TextureSlot;
static  GLuint  m_GrayColor_ProjectionModelviewSlot;
static  GLuint  m_GrayColor_PositionSlot;
static  GLuint  m_GrayColor_ColorSlot;
static  GLuint  m_GrayColor_TexCoordSlot;
//-----------------------------------------------------------------------------
static  bool 	gl_shader2d_program_gray_color_init(void)
{
    if(gl_shader_build_alloc((char *)m_default_vertex_2d_source, (char *)m_gray_color_fragment_2d_source,
                             (GLuint *)&m_idGrayColor_VertexShader, (GLuint *)&m_idGrayColor_FragmentShader, 
                             (GLuint *)&m_idGrayColor_ShaderProgram) == false) 
    {
        return false;
    }
    
    glUseProgram(m_idGrayColor_ShaderProgram);
    
    m_GrayColor_TextureSlot = glGetUniformLocation(m_idGrayColor_ShaderProgram, "u_Texture");
    glUniform1i(m_GrayColor_TextureSlot, 0);
    
    m_GrayColor_ProjectionModelviewSlot = glGetUniformLocation(m_idGrayColor_ShaderProgram, "u_ProjectionModelview");
    glUniformMatrix4fv(m_GrayColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
    
    m_GrayColor_PositionSlot = glGetAttribLocation(m_idGrayColor_ShaderProgram, "a_Position");
    m_GrayColor_ColorSlot = glGetAttribLocation(m_idGrayColor_ShaderProgram, "a_Color");
    m_GrayColor_TexCoordSlot = glGetAttribLocation(m_idGrayColor_ShaderProgram, "a_TexCoord");
    
    return true;
}
//-----------------------------------------------------------------------------
static  void    gl_shader2d_program_gray_color_end(void)
{
    gl_shader_build_free(m_idGrayColor_ShaderProgram, m_idGrayColor_VertexShader, m_idGrayColor_FragmentShader);
}
//-----------------------------------------------------------------------------
#define DEF_MAX_DRAW_ARRAY_2D   (362)

static  float   m_fDefaultVertex[2 * 4 * DEF_MAX_DRAW_ARRAY_2D];
static  float   m_fDefaultColor[4 * 4 * DEF_MAX_DRAW_ARRAY_2D];
static  float   m_fDefaultTexCoord[2 * 4 * DEF_MAX_DRAW_ARRAY_2D];
static  float   m_fZeroTexCoord[2 * 4 * DEF_MAX_DRAW_ARRAY_2D];
static  float   *m_pfCurrentTexCoord;
//-----------------------------------------------------------------------------
static  u32bits 	m_UseProgramType;
//-----------------------------------------------------------------------------
static  void    gl_shader2d_use_program_disable(void)
{
	switch(m_UseProgramType)
	{
		case    DEF_USE_PROGRAM_TYPE_GRAY_COLOR:
			glDisableVertexAttribArray(m_GrayColor_PositionSlot);
			glDisableVertexAttribArray(m_GrayColor_TexCoordSlot);
			glDisableVertexAttribArray(m_GrayColor_ColorSlot);
			break;

		case    DEF_USE_PROGRAM_TYPE_NEGATIVE_COLOR:
			glDisableVertexAttribArray(m_NegativeColor_PositionSlot);
			glDisableVertexAttribArray(m_NegativeColor_TexCoordSlot);
			glDisableVertexAttribArray(m_NegativeColor_ColorSlot);
			break;
		
		case    DEF_USE_PROGRAM_TYPE_ASSIGN_COLOR:
			glDisableVertexAttribArray(m_AssignColor_PositionSlot);
			glDisableVertexAttribArray(m_AssignColor_TexCoordSlot);
			glDisableVertexAttribArray(m_AssignColor_ColorSlot);
			break;
						
		case    DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR:
			glDisableVertexAttribArray(m_DefaultVertexColor_PositionSlot);
			glDisableVertexAttribArray(m_DefaultVertexColor_TexCoordSlot);
			glDisableVertexAttribArray(m_DefaultVertexColor_ColorSlot);
			break;
			
		case    DEF_USE_PROGRAM_TYPE_DEFAULT:
		default:
			glDisableVertexAttribArray(m_Default_PositionSlot);
			glDisableVertexAttribArray(m_Default_TexCoordSlot);
			glDisableVertexAttribArray(m_Default_ColorSlot);
			break;
	}
	
    //glUseProgram(0);
    
    m_UseProgramType = DEF_USE_PROGRAM_TYPE_NULL;    
}
//-----------------------------------------------------------------------------
static  void    gl_shader2d_use_program(u32bits ProgramType)
{
    GLint   ProjectModelview;
	
    gl_shader2d_use_program_disable();

	if(m_UseProgramType == ProgramType)
	{
		switch(ProgramType)
		{
			case    DEF_USE_PROGRAM_TYPE_GRAY_COLOR:
				glVertexAttribPointer(m_GrayColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
				
				ProjectModelview = m_GrayColor_ProjectionModelviewSlot;
				break;
				
			case    DEF_USE_PROGRAM_TYPE_NEGATIVE_COLOR:
				glVertexAttribPointer(m_NegativeColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
			
				ProjectModelview = m_NegativeColor_ProjectionModelviewSlot;
				break;
				
			case    DEF_USE_PROGRAM_TYPE_ASSIGN_COLOR:
				glUniform4f(m_AssignColor_AssignColorSlot, m_AssignR, m_AssignG, m_AssignB, 0xFF);
				
				glVertexAttribPointer(m_AssignColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
				
				ProjectModelview = m_AssignColor_ProjectionModelviewSlot;
				break;
				
			case    DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR:
				glVertexAttribPointer(m_DefaultVertexColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
				
				ProjectModelview = m_DefaultVertexColor_ProjectionModelviewSlot;
				break;
				
			case    DEF_USE_PROGRAM_TYPE_DEFAULT:
			default:
				glVertexAttribPointer(m_Default_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
				
				ProjectModelview = m_Default_ProjectionModelviewSlot;
				break;
		}
		
		if(m_bFlipY != m_bPrevFlipY)
		{
			m_bPrevFlipY = m_bFlipY;
			if(m_bFlipY)
			{
				glUniformMatrix4fv(ProjectModelview, 1, 0, &m_ProjectionModelviewFlipY[0]);
			}
			else
			{
				glUniformMatrix4fv(ProjectModelview, 1, 0, &m_ProjectionModelview[0]);
			}
		}
		return;
	}

	switch(ProgramType)
	{
		case    DEF_USE_PROGRAM_TYPE_GRAY_COLOR:
			glUseProgram(m_idGrayColor_ShaderProgram);
			
			glUniform1i(m_GrayColor_TextureSlot, 0);
			
			glVertexAttribPointer(m_GrayColor_PositionSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultVertex);
			glVertexAttribPointer(m_GrayColor_ColorSlot, 4, GL_FLOAT, GL_FALSE, 0, m_fDefaultColor);
			//glVertexAttribPointer(m_GrayColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultTexCoord);
			glVertexAttribPointer(m_GrayColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
			
			glEnableVertexAttribArray(m_GrayColor_PositionSlot);
			glEnableVertexAttribArray(m_GrayColor_ColorSlot);
			glEnableVertexAttribArray(m_GrayColor_TexCoordSlot);
			
			if(m_bFlipY)
			{
				glUniformMatrix4fv(m_GrayColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelviewFlipY[0]);
			}
			else
			{
				glUniformMatrix4fv(m_GrayColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
			}
			break;
			
		case    DEF_USE_PROGRAM_TYPE_NEGATIVE_COLOR:
			glUseProgram(m_idNegativeColor_ShaderProgram);
			
			glUniform1i(m_NegativeColor_TextureSlot, 0);
			
			glVertexAttribPointer(m_NegativeColor_PositionSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultVertex);
			glVertexAttribPointer(m_NegativeColor_ColorSlot, 4, GL_FLOAT, GL_FALSE, 0, m_fDefaultColor);
			//glVertexAttribPointer(m_NegativeColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultTexCoord);
			glVertexAttribPointer(m_NegativeColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
			
			glEnableVertexAttribArray(m_NegativeColor_PositionSlot);
			glEnableVertexAttribArray(m_NegativeColor_ColorSlot);
			glEnableVertexAttribArray(m_NegativeColor_TexCoordSlot);
			
			if(m_bFlipY)
			{
				glUniformMatrix4fv(m_NegativeColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelviewFlipY[0]);
			}
			else
			{
				glUniformMatrix4fv(m_NegativeColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
			}
			break;
			
		case    DEF_USE_PROGRAM_TYPE_ASSIGN_COLOR:
			glUseProgram(m_idAssignColor_ShaderProgram);
			
			glUniform1i(m_AssignColor_TextureSlot, 0);
			
			glUniform4f(m_AssignColor_AssignColorSlot, m_AssignR, m_AssignG, m_AssignB, 0xFF);
			
			glVertexAttribPointer(m_AssignColor_PositionSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultVertex);
			glVertexAttribPointer(m_AssignColor_ColorSlot, 4, GL_FLOAT, GL_FALSE, 0, m_fDefaultColor);
			//glVertexAttribPointer(m_AssignColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultTexCoord);
			glVertexAttribPointer(m_AssignColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
			
			glEnableVertexAttribArray(m_AssignColor_PositionSlot);
			glEnableVertexAttribArray(m_AssignColor_ColorSlot);
			glEnableVertexAttribArray(m_AssignColor_TexCoordSlot);
			
			if(m_bFlipY)
			{
				glUniformMatrix4fv(m_AssignColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelviewFlipY[0]);
			}
			else
			{
				glUniformMatrix4fv(m_AssignColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
			}
			break;

		case    DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR:
			glUseProgram(m_idDefaultVertexColor_ShaderProgram);
			
			glUniform1i(m_DefaultVertexColor_TextureSlot, 0);
			
			glVertexAttribPointer(m_DefaultVertexColor_PositionSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultVertex);
			glVertexAttribPointer(m_DefaultVertexColor_ColorSlot, 4, GL_FLOAT, GL_FALSE, 0, m_fDefaultColor);
			//glVertexAttribPointer(m_DefaultVertexColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultTexCoord);
			glVertexAttribPointer(m_DefaultVertexColor_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
			
			glEnableVertexAttribArray(m_DefaultVertexColor_PositionSlot);
			glEnableVertexAttribArray(m_DefaultVertexColor_ColorSlot);
			glEnableVertexAttribArray(m_DefaultVertexColor_TexCoordSlot);
			
			if(m_bFlipY)
			{
				glUniformMatrix4fv(m_DefaultVertexColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelviewFlipY[0]);
			}
			else
			{
				glUniformMatrix4fv(m_DefaultVertexColor_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
			}
			break;
		
		case    DEF_USE_PROGRAM_TYPE_DEFAULT:
		default:
			glUseProgram(m_idDefault_ShaderProgram);
			
			glUniform1i(m_Default_TextureSlot, 0);
			
			glVertexAttribPointer(m_Default_PositionSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultVertex);
			glVertexAttribPointer(m_Default_ColorSlot, 4, GL_FLOAT, GL_FALSE, 0, m_fDefaultColor);
			//glVertexAttribPointer(m_Default_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_fDefaultTexCoord);
			glVertexAttribPointer(m_Default_TexCoordSlot, 2, GL_FLOAT, GL_FALSE, 0, m_pfCurrentTexCoord);
			
			glEnableVertexAttribArray(m_Default_PositionSlot);
			glEnableVertexAttribArray(m_Default_ColorSlot);
			glEnableVertexAttribArray(m_Default_TexCoordSlot);
			
			if(m_bFlipY)
			{
				glUniformMatrix4fv(m_Default_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelviewFlipY[0]);
			}
			else
			{
				glUniformMatrix4fv(m_Default_ProjectionModelviewSlot, 1, 0, &m_ProjectionModelview[0]);
			}
			break;
	}
	
	m_UseProgramType = ProgramType;
}
//-----------------------------------------------------------------------------
static  const   u8bits   ascii_16x16_table[]=
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x03,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0xD8,0x00,0xD8,0x00,0xF0,0x07,0xFC,0x07,0xF8,0x01,0xB0,0x07,0xF8,0x07,0xF8,0x03,0x60,0x03,0xC0,0x06,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x40,0x01,0xF8,0x03,0xF8,0x03,0x40,0x03,0xC0,0x03,0xC0,0x01,0xF0,0x00,0x78,0x00,0x78,0x00,0x58,0x03,0x78,0x03,0xF0,0x00,0x40,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0F,0x18,0x0F,0xB8,0x09,0xB0,0x0D,0xE0,0x0F,0xC0,0x07,0xC0,0x01,0xF8,0x03,0xD8,0x07,0xC8,0x06,0xD8,0x0C,0x78,0x0C,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0xF0,0x01,0xF0,0x01,0xF0,0x01,0xF0,0x03,0xE0,0x07,0xCC,0x06,0xEC,0x06,0x7C,0x06,0x7C,0x07,0x78,0x03,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x18,0x00,0x78,0x00,0xE0,0x01,0xC0,0x01,0xC0,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0xC0,0x00,0xC0,0x00,0xF0,0x00,0x78,0x00,0x18,
    0x00,0x00,0x06,0x00,0x07,0x80,0x03,0x80,0x01,0xC0,0x00,0xC0,0x00,0xE0,0x00,0x60,0x00,0x60,0x00,0xE0,0x00,0xE0,0x00,0xC0,0x01,0xC0,0x03,0x80,0x07,0x00,0x04,0x00,
    0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0xC0,0x03,0xF8,0x03,0xF0,0x01,0xE0,0x01,0xF0,0x01,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x07,0xF8,0x07,0xF8,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xC0,0x00,0x80,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x18,0x00,0x18,0x00,0x30,0x00,0x30,0x00,0x60,0x00,0x60,0x00,0xC0,0x00,0xC0,0x01,0x80,0x01,0x80,0x03,0x00,0x03,0x00,0x06,0x00,0x06,0x00,0x04,0x00,
    0x00,0x00,0x00,0x00,0x03,0xE0,0x07,0x70,0x06,0x30,0x06,0x30,0x06,0x38,0x06,0x38,0x06,0x38,0x06,0x38,0x06,0x30,0x07,0x70,0x03,0xE0,0x01,0xC0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0xE0,0x03,0xE0,0x02,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x03,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0x70,0x00,0x38,0x00,0x38,0x00,0x30,0x00,0x70,0x00,0xE0,0x01,0xC0,0x03,0x80,0x03,0xF8,0x03,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xE0,0x03,0x70,0x00,0x30,0x00,0x70,0x03,0xE0,0x03,0xE0,0x00,0x70,0x00,0x30,0x00,0x30,0x02,0x70,0x03,0xF0,0x03,0xC0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x60,0x00,0xE0,0x01,0xE0,0x01,0xE0,0x03,0x60,0x07,0x60,0x06,0x60,0x07,0xF8,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF0,0x03,0x00,0x03,0x00,0x03,0xC0,0x03,0xE0,0x00,0x70,0x00,0x30,0x00,0x70,0x02,0x70,0x03,0xE0,0x03,0x80,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xB0,0x06,0x00,0x06,0x00,0x07,0xE0,0x07,0x70,0x06,0x30,0x06,0x30,0x06,0x30,0x07,0x70,0x03,0xE0,0x01,0xC0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF8,0x03,0xF8,0x00,0x30,0x00,0x70,0x00,0x60,0x00,0xE0,0x00,0xC0,0x01,0xC0,0x01,0x80,0x03,0x80,0x03,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x07,0x70,0x06,0x30,0x07,0x70,0x03,0xE0,0x03,0xE0,0x07,0xF0,0x06,0x30,0x06,0x30,0x07,0x70,0x07,0xF0,0x01,0xC0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xE0,0x07,0x70,0x06,0x30,0x06,0x30,0x06,0x30,0x07,0x70,0x07,0xF0,0x01,0xB0,0x00,0x30,0x04,0x70,0x07,0xE0,0x03,0x80,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xC0,0x00,0x80,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x78,0x01,0xF0,0x07,0xC0,0x07,0x80,0x01,0xE0,0x00,0x78,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xF8,0x07,0xF8,0x00,0x00,0x07,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x07,0x00,0x03,0xC0,0x00,0xF0,0x00,0xF8,0x03,0xE0,0x07,0x80,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF8,0x03,0x38,0x02,0x18,0x00,0x38,0x00,0x70,0x00,0xE0,0x00,0xC0,0x00,0xC0,0x00,0x00,0x01,0xC0,0x01,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0xF0,0x03,0xB0,0x07,0x78,0x06,0xD8,0x07,0x98,0x05,0xB8,0x05,0xB8,0x06,0xFC,0x06,0xFC,0x07,0xB0,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0xC0,0x01,0xC0,0x03,0xE0,0x03,0xE0,0x03,0x60,0x07,0x70,0x06,0x70,0x07,0xF0,0x0E,0x38,0x0C,0x38,0x0C,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF8,0x03,0x38,0x03,0x38,0x03,0x70,0x03,0xF0,0x03,0x78,0x03,0x38,0x03,0x18,0x03,0x38,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0xF8,0x03,0x98,0x07,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x07,0x00,0x03,0x88,0x03,0xF8,0x00,0xF0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF8,0x03,0x38,0x03,0x1C,0x03,0x1C,0x03,0x1C,0x03,0x1C,0x03,0x18,0x03,0x18,0x03,0x78,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF8,0x03,0xF8,0x03,0x00,0x03,0x00,0x03,0xF0,0x03,0xF0,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF8,0x03,0xF8,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0xF0,0x03,0xF0,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0xF8,0x03,0x98,0x07,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x18,0x06,0x18,0x07,0x18,0x03,0x98,0x03,0xF8,0x00,0xF0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0xF8,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF8,0x03,0xF8,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x03,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF0,0x00,0x70,0x00,0x70,0x00,0x70,0x00,0x70,0x00,0x70,0x00,0x70,0x00,0x70,0x02,0x60,0x03,0xE0,0x01,0x80,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x38,0x03,0x30,0x03,0x70,0x03,0xE0,0x03,0xC0,0x03,0xC0,0x03,0xE0,0x03,0xF0,0x03,0x70,0x03,0x38,0x03,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x06,0x38,0x07,0x38,0x07,0x38,0x07,0x78,0x07,0xF8,0x05,0xF8,0x05,0xF8,0x05,0xD8,0x04,0x18,0x04,0x18,0x04,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x18,0x03,0x98,0x03,0x98,0x03,0xD8,0x03,0xD8,0x03,0xF8,0x03,0xF8,0x03,0x78,0x03,0x78,0x03,0x38,0x03,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x07,0xF8,0x07,0x38,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x07,0x38,0x03,0xF0,0x00,0xC0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF8,0x03,0x18,0x03,0x18,0x03,0x38,0x03,0xF8,0x03,0xE0,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x07,0xF8,0x07,0x38,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x07,0x38,0x03,0xF0,0x00,0xF8,0x00,0x3C,0x00,0x08,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF8,0x03,0x38,0x03,0x38,0x03,0x38,0x03,0xF0,0x03,0xE0,0x03,0x70,0x03,0x38,0x03,0x38,0x03,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xB0,0x03,0x00,0x03,0x00,0x03,0xC0,0x01,0xF0,0x00,0x78,0x00,0x38,0x00,0x18,0x03,0x38,0x03,0xF0,0x01,0xE0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0F,0xF8,0x0F,0xF8,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0xB8,0x03,0xF0,0x00,0xE0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0C,0x18,0x0E,0x18,0x0E,0x38,0x06,0x30,0x07,0x30,0x07,0x70,0x03,0x60,0x03,0xE0,0x03,0xE0,0x01,0xC0,0x01,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0C,0x18,0x0C,0x18,0x0C,0x18,0x0D,0xD8,0x0D,0xD8,0x0D,0xD0,0x0F,0xF0,0x07,0xF0,0x07,0x70,0x07,0x70,0x07,0x70,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0E,0x38,0x06,0x30,0x07,0x70,0x03,0xE0,0x03,0xC0,0x01,0xC0,0x03,0xC0,0x03,0xE0,0x07,0x70,0x0E,0x70,0x0E,0x38,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x0E,0x18,0x0E,0x38,0x07,0x30,0x07,0x70,0x03,0xE0,0x01,0xC0,0x01,0xC0,0x01,0xC0,0x01,0xC0,0x01,0xC0,0x01,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x07,0xF8,0x07,0xF8,0x00,0x70,0x00,0x70,0x00,0xE0,0x01,0xC0,0x01,0xC0,0x03,0x80,0x07,0x00,0x07,0x00,0x07,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xF8,0x00,0xF8,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xF8,0x00,0xF8,
    0x00,0x00,0x06,0x00,0x06,0x00,0x07,0x00,0x03,0x00,0x03,0x80,0x01,0x80,0x01,0xC0,0x00,0xC0,0x00,0xE0,0x00,0x60,0x00,0x70,0x00,0x30,0x00,0x38,0x00,0x18,0x00,0x18,
    0x00,0x00,0x07,0xC0,0x07,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x07,0xC0,0x07,0xC0,
    0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xE0,0x01,0xE0,0x01,0xE0,0x03,0x30,0x03,0x30,0x06,0x18,0x06,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xF8,0x00,0x00,0x00,0x00,
    0x00,0x00,0x01,0x80,0x01,0xC0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xE0,0x07,0x70,0x00,0x70,0x03,0xF0,0x07,0x70,0x06,0x70,0x07,0xF0,0x07,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x07,0xF0,0x07,0xF0,0x06,0x38,0x06,0x38,0x06,0x38,0x06,0x30,0x07,0x70,0x07,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xF8,0x03,0xD8,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0xD8,0x01,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x03,0xF0,0x07,0x70,0x06,0x30,0x06,0x30,0x06,0x30,0x06,0x70,0x07,0xF0,0x07,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF0,0x07,0x70,0x06,0x30,0x07,0xF0,0x07,0xF0,0x06,0x00,0x07,0xB0,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xF8,0x01,0xF8,0x01,0x80,0x01,0x80,0x07,0xF8,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF0,0x07,0x70,0x06,0x30,0x06,0x30,0x06,0x30,0x06,0x70,0x07,0xF0,0x03,0xF0,0x00,0x30,0x02,0x70,0x07,0xE0,
    0x00,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0xF0,0x03,0xF8,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x03,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x07,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x04,0xC0,0x07,0xC0,
    0x00,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03,0x38,0x03,0x70,0x03,0xE0,0x03,0xC0,0x03,0xE0,0x03,0xF0,0x03,0x78,0x03,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x03,0xE0,0x03,0xE0,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xF8,0x07,0xF8,0x06,0xD8,0x06,0xD8,0x06,0xD8,0x06,0xD8,0x06,0xD8,0x06,0xD8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF8,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF0,0x07,0x70,0x06,0x38,0x06,0x18,0x06,0x18,0x06,0x38,0x07,0x70,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF0,0x03,0xF8,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0xB8,0x03,0xF0,0x03,0x00,0x03,0x00,0x03,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF0,0x07,0x70,0x06,0x30,0x06,0x30,0x06,0x30,0x06,0x30,0x07,0xF0,0x03,0xF0,0x00,0x30,0x00,0x30,0x00,0x30,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xF8,0x01,0xF8,0x01,0xD8,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF8,0x03,0x98,0x03,0x80,0x03,0xF0,0x01,0xF8,0x00,0x38,0x03,0x38,0x03,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x01,0x80,0x07,0xF0,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0x80,0x01,0xC0,0x01,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x18,0x03,0x38,0x03,0xF8,0x03,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x18,0x07,0x18,0x07,0x38,0x03,0xB0,0x03,0xF0,0x01,0xE0,0x01,0xE0,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0xD8,0x0D,0xD8,0x0D,0xD8,0x0D,0xF8,0x0F,0x78,0x07,0x70,0x07,0x70,0x07,0x70,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x38,0x07,0xF0,0x03,0xE0,0x01,0xE0,0x01,0xE0,0x03,0xF0,0x07,0x70,0x06,0x38,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x18,0x07,0x18,0x03,0x38,0x03,0xB0,0x03,0xF0,0x01,0xE0,0x01,0xE0,0x00,0xC0,0x00,0xC0,0x01,0x80,0x07,0x80,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xF0,0x00,0x70,0x00,0xE0,0x01,0xC0,0x01,0xC0,0x03,0x80,0x07,0x00,0x07,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x70,0x00,0xF0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x01,0xC0,0x03,0x80,0x01,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xF0,0x00,0x70,
    0x00,0x00,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,
    0x00,0x00,0x03,0x80,0x03,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xE0,0x00,0x70,0x00,0xE0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x03,0xC0,0x03,0x80,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x07,0xD8,0x06,0xF8,0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
//-----------------------------------------------------------------------------
#define DEF_MAX_ASCII_WORD 	(16 * 6)
/*
     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
 0      !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
 1   0  1  2  3  4  5  6  7  8  9  :  ;  <  -  >  ?
 2   @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O
 3   P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _
 4   '  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o
 5   p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~
*/

typedef struct  tagUV_POS
{
    float   tl, tu, tr, td; // 貼圖的 uv 座標
} UV_POS;

static  GLuint  m_idAsciiTexture;
static  UV_POS  m_AsciiTexCoord[DEF_MAX_ASCII_WORD];
//-----------------------------------------------------------------------------
static  bool    gl_shader2d_ascii_texture_init(void)
{
    u8bits  tmp_font_buf[16 * 16 * sizeof(u32bits)];
    u32bits x, y, w, h, shift, index;
    u32bits *pdw;
    u8bits  *buf;
    
    u32bits  *pbit;
    u32bits  size;
    u32bits  xx, yy;
    u32bits  iw, ih;
    u32bits  tw, th;
    float    u, v, u_step, v_step;
    u32bits  *pSrc, *pDst;
    UV_POS   *uv;
    
    w = 16;
    h = 16;
    
    iw = w * 16;
    ih = h * 6;
    
    tw = 1;
    th = 1;
    while(tw < iw) { tw <<= 1; }
    while(th < ih) { th <<= 1; }

    size = sizeof(u32bits) * tw * th;
    pbit = (u32bits *)malloc(size);
    if(pbit == null)
    {
        return false;
    }
    memset(pbit, 0, size);
    
    u = 0;
    v = 0;
    u_step = (float)w / (float)tw;
    v_step = (float)h / (float)th;
    
    xx = 0;
    yy = 0;
    
    for(index=0; index<DEF_MAX_ASCII_WORD; index++)
    {
        buf = (u8bits *)(ascii_16x16_table + (index * 32));
        
        pdw = (u32bits *)tmp_font_buf;
        shift = 0;
        for(y=0; y<16; y++)
        {
            for(x=0; x<16; x++)
            {
                if(shift > 7)
                {
                    shift = 0;
                    buf++;
                }
                if((*buf) & (0x80 >> shift))
                {
                    *pdw = 0xFFFFFFFF;
                    pdw++;
                }
                else
                {
                    *pdw = 0;
                    pdw++;
                }
                shift++;
            }
        }
        
        pSrc = (u32bits *)tmp_font_buf;
        for(y=0; y<16; y++)
        {
            pDst = (u32bits *)pbit + (yy + y) * tw + xx;
            for(x=0; x<16; x++)
            {
                *pDst = *pSrc;
                pSrc++;
                pDst++;
            }
        }
        uv = (UV_POS *)(m_AsciiTexCoord + index);
        uv->tl = u;
        uv->tu = v;
        uv->tr = uv->tl + u_step;
        uv->td = uv->tu + v_step;
        
        xx += 16;
        u += u_step;
        if(xx >= iw)
        {
            xx = 0;
            yy += 16;
            
            u = 0;
            v += v_step;
        }
    }
    
	glGenTextures(1, &m_idAsciiTexture);
	glBindTexture(GL_TEXTURE_2D, m_idAsciiTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, pbit);	
	
    free(pbit);
    
    return true;
}
//-----------------------------------------------------------------------------
static  void    gl_shader2d_ascii_texture_end(void)
{
	if(glIsTexture(m_idAsciiTexture))
	{
		glDeleteTextures(1, (const GLuint*)&m_idAsciiTexture);
	}
}
//-----------------------------------------------------------------------------
#include    <stdarg.h>

static  void    gl_shader2d_draw_texture_ascii(s32bits x, s32bits y, u8bits r, u8bits g, u8bits b, u8bits a, u32bits index)
{
    UV_POS  *uv;
    float   *f, *v, *c, *t;
    float   fr, fg, fb, fa;
    float   x1, y1, x2, y2;
    float   tl, tu, tr, td;
    
    x1 = (float)x;
    y1 = (float)y;
    x2 = (float)(x + 16);
    y2 = (float)(y + 16);
	
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);    
	
    v = (float *)m_fDefaultVertex;
    *(v    ) = x1;
    *(v + 1) = y1;
    *(v + 2) = x2;
    *(v + 3) = y1;
    *(v + 4) = x1;
    *(v + 5) = y2;
    *(v + 6) = x2;
    *(v + 7) = y2;
    
    c = (float *)m_fDefaultColor;
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
    *(c + 4) = fr;
    *(c + 5) = fg;
    *(c + 6) = fb;
    *(c + 7) = fa;
	*(c + 8) = fr;
	*(c + 9) = fg;
	*(c +10) = fb;
	*(c +11) = fa;
	*(c +12) = fr;
	*(c +13) = fg;
	*(c +14) = fb;
	*(c +15) = fa;
    
    uv = (UV_POS *)(m_AsciiTexCoord + index);
    tl = uv->tl;
    tu = uv->tu;
    tr = uv->tr;
    td = uv->td;
	m_pfCurrentTexCoord = m_fDefaultTexCoord;
    t = (float *)m_fDefaultTexCoord;
    *(t    ) = tl;
    *(t + 1) = tu;
    *(t + 2) = tr;
    *(t + 3) = tu;
    *(t + 4) = tl;
    *(t + 5) = td;
    *(t + 6) = tr;
    *(t + 7) = td;
    
    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idAsciiTexture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_printf(s32bits x, s32bits y, u8bits r, u8bits g, u8bits b, u8bits a, char *szFormat, ...)
{
    va_list         args;
    char            szText[256], *szString;
    unsigned short  wHighByte;
    u32bits    w;

    w = 9;
    
    va_start(args, szFormat);
#if defined(WIN32)
    vsprintf_s((char *)szText, sizeof(szText), (char *)szFormat, args);
#else    
    vsprintf((char *)szText, (char *)szFormat, args);
#endif
    va_end(args);
    
    szString = szText;
    
    while(*szString)
    {
        wHighByte = szString[0] & 0x00FF;
        
        if((wHighByte == '\n') || (wHighByte == '\r') || (wHighByte == '\t'))
        {
            continue;
        }
        
        wHighByte -= ' ';
        
        gl_shader2d_draw_texture_ascii(x, y, r, g, b, a, wHighByte);

        x += w;
        szString++;
    }
}
//-----------------------------------------------------------------------------
static  void    gl_shader2d_draw_flip_y(bool bFlag)
{
    m_bPrevFlipY = m_bFlipY;
    m_bFlipY = bFlag;
}
//-----------------------------------------------------------------------------
RENDER_TEXTURE	*gl_shader2d_fbo_alloc(u32bits w, u32bits h)
{
    RENDER_TEXTURE	*rt;
    GLuint          texID;
    u32bits        	tw, th;
    float          	u, v;
    GLenum         	status;
    GLint          	idOldFBO;

     
    rt = (RENDER_TEXTURE *)malloc(sizeof(RENDER_TEXTURE));
    if(rt == null)
    {
        return null;
    }
    memset(rt, 0, sizeof(RENDER_TEXTURE));
    
    // 貼圖是否需要 2 的次方判斷
    if(non_power_of_two)
    {
        tw = w;
        th = h;        
    }
    else
    {
        tw = 1;
        th = 1;
        while(tw < w) { tw <<= 1; }
        while(th < h) { th <<= 1; }
    }
    
    u = (float)w / (float)tw;
    v = (float)h / (float)th;
    
    // 建立貼圖
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);	
	
    rt->texID = texID;
    rt->tl    = 0.0f;
    rt->tu    = 0.0f;
    rt->tr    = u;
    rt->td    = v;
    rt->iw    = w;
    rt->ih    = h;
    rt->tw    = tw;
    rt->th    = th;
    rt->size  = tw * th * sizeof(u32bits);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &idOldFBO); // 記錄舊的 FBO
    
    // 建立 frame buffer
    glGenFramebuffers(1, &rt->idFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, rt->idFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->texID, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
	
	#if 0 // 深度 buffer
	glGenRenderbuffers(1, &rt->idRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, rt->idRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, tw, th);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->idRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	#endif
	
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            //printf("Framebuffer complete.\n");
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            printf("[ERROR] Framebuffer incomplete: Attachment is NOT complete.\n");
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            printf("[ERROR] Framebuffer incomplete: No image is attached to FBO.\n");
            break;
            
        //case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        //    printf("[ERROR] Framebuffer incomplete: Attached images have different dimensions.\n");
        //    break;
            
        case GL_FRAMEBUFFER_UNSUPPORTED:
            printf("[ERROR] Unsupported by FBO implementation.\n");
            break;
            
        default:
            printf("[ERROR] Unknow error.\n");
            break;
    }
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, idOldFBO); // 復原舊的 FBO  
        
		#if 0 // 深度 buffer
		glDeleteRenderbuffers(1, &rt->idRBO);
		#endif
         
        glDeleteFramebuffers(1, &rt->idFBO);
 
		if(glIsTexture(rt->texID))
		{
			glDeleteTextures(1, (const GLuint*)&rt->texID);
		}
	
        free(rt);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, idOldFBO); // 復原舊的 FBO           
        return rt;
    }
    return null;
}
//-----------------------------------------------------------------------------
void    gl_shader2d_fbo_free(RENDER_TEXTURE *rt)
{
    if(rt == null) return;
    
#if 0 // 深度 buffer
	glDeleteRenderbuffers(1, &rt->idRBO);
#endif

	glDeleteFramebuffers(1, &rt->idFBO);

	if(glIsTexture(rt->texID))
	{
		glDeleteTextures(1, (const GLuint*)&rt->texID);
	}
    
	free(rt);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_fbo_draw_begin(RENDER_TEXTURE *rt, u8bits r, u8bits g, u8bits b, u8bits a)
{
    GLbitfield  clear_mask;
    float       *f;
    float       fr, fg, fb, fa;
    
    if(rt == null) return;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&rt->idOldFBO);
    
    glBindFramebuffer(GL_FRAMEBUFFER, rt->idFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->texID, 0);
    
    clear_mask = GL_COLOR_BUFFER_BIT;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
    
    glViewport(0, 0, m_ViewWidth, m_ViewHeight);

    glClearColor(fr, fg, fb, fa);
    glClear(clear_mask);
    
    gl_shader2d_draw_flip_y(true);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_fbo_draw_end(RENDER_TEXTURE *rt)
{
    gl_shader2d_draw_flip_y(false);
    
    glBindFramebuffer(GL_FRAMEBUFFER, rt->idOldFBO);    
}
//-----------------------------------------------------------------------------
void    gl_shader2d_fbo_draw_texture(RENDER_TEXTURE *rt,
									 u32bits program_type,
									 s32bits x, s32bits y,
									 u32bits w, u32bits h,
									 u8bits r, u8bits g, u8bits b, u8bits a, 
									 bool h_mirror, bool v_mirror,
									 s32bits angle)
{
    if(rt == null) return;
    
    gl_shader2d_draw_texture(program_type, x, y, w, h, r, g, b, a, h_mirror, v_mirror, angle, rt->texID, rt->tl, rt->tu, rt->tr, rt->td);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_fbo_texture_pixel_get(RENDER_TEXTURE *rt, u8bits *buffer, u32bits x, u32bits y, u32bits w, u32bits h)
{
    s32bits     vp[4];

    if(rt == null) return;
    
    glBindFramebuffer(GL_FRAMEBUFFER, rt->idFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->texID, 0);
    
    glGetIntegerv(GL_VIEWPORT, vp);

    glViewport(0, 0, m_ViewWidth, m_ViewHeight);
    
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(vp[0], vp[1], vp[2], vp[3]);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static  GLuint  m_idDefaultTexture;
//-----------------------------------------------------------------------------
static  void    gl_shader2d_default_texture_init(void)
{
    u32bits	tmp_buf[4];
    u32bits iw, ih, tw, th;
    
    tmp_buf[0] = 0xFFFFFFFF;
    tmp_buf[1] = 0xFFFFFFFF;
    tmp_buf[2] = 0xFFFFFFFF;
    tmp_buf[3] = 0xFFFFFFFF;

    // 貼圖是否需要 2 的次方判斷
    if(non_power_of_two)
    {
		iw = 1;
		ih = 1;

        tw = iw;
        th = ih;        
    }
    else
    {
		iw = 2;
		ih = 2;

        tw = 1;
        th = 1;
        while(tw < iw) { tw <<= 1; }
        while(th < ih) { th <<= 1; }
    }

	glGenTextures(1, &m_idDefaultTexture);
	glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp_buf);	
}
//-----------------------------------------------------------------------------
static  void    gl_shader2d_default_texture_end(void)
{
	if(glIsTexture(m_idDefaultTexture))
	{
		glDeleteTextures(1, (const GLuint*)&m_idDefaultTexture);
	}
}
//-----------------------------------------------------------------------------
void    gl_shader2d_clip_on(s32bits x, s32bits y, u32bits w, u32bits h)
{
    s32bits  start_y;

    start_y = m_ViewHeight - y;
    glScissor(x, start_y - h, w, h);
    glEnable(GL_SCISSOR_TEST);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_clip_off(void)
{
    glDisable(GL_SCISSOR_TEST);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_begin(void)
{
	//GLint 	viewport[4];
	//glGetIntegerv(GL_VIEWPORT, viewport);	
	
    glPushAttrib(GL_ENABLE_BIT);
	
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
	
    glEnable(GL_TEXTURE_2D);
  
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glViewport(0, 0, m_ViewWidth, m_ViewHeight);    
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_end(void)
{      
    gl_shader2d_use_program_disable();
    glUseProgram(0);
	
    glPopAttrib();
}
//-----------------------------------------------------------------------------
static  float   m_fBackR, m_fBackG, m_fBackB;
//---------------------------------------------------------------------------
void    gl_shader2d_background_color_set(u8bits r, u8bits g, u8bits b)
{
    float   *f;
    
    f = (float *)g_fByteToFloatTable;
    m_fBackR = *(f + r);
    m_fBackG = *(f + g);
    m_fBackB = *(f + b);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_clear(void)
{
    glClearColor(m_fBackR, m_fBackG, m_fBackB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_pixel(s32bits x, s32bits y, u8bits r, u8bits g, u8bits b, u8bits a)
{
    float   *f, *v, *c;//, *t;    
    float   fr, fg, fb, fa;
	
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);

	// 頂點
    v = (float *)m_fDefaultVertex;
    *(v    ) = (float)x;
    *(v + 1) = (float)y;
	// 顏色
    c = (float *)m_fDefaultColor;
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
	// 貼圖座標
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = (float *)m_fDefaultTexCoord;
    *(t    ) = 0.0f;
    *(t + 1) = 0.0f;
    *(t + 2) = 0.0f;
    *(t + 3) = 0.0f;
#endif

    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_POINTS, 0, 1);  
    glBindTexture(GL_TEXTURE_2D, 0);
 }
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_line(s32bits x1, s32bits y1, s32bits x2, s32bits y2, u8bits r, u8bits g, u8bits b, u8bits a)
{
    float   *f, *v, *c;//, *t;
    float   fr, fg, fb, fa;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
	
    // 頂點
    v = (float *)m_fDefaultVertex;
    *(v    ) = (float)x1;
    *(v + 1) = (float)y1;
    *(v + 2) = (float)x2;
    *(v + 3) = (float)y2;
	// 顏色
    c = (float *)m_fDefaultColor;
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
    *(c + 4) = fr;
    *(c + 5) = fg;
    *(c + 6) = fb;
    *(c + 7) = fa;    
	// 貼圖座標
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = (float *)m_fDefaultTexCoord;
    *(t    ) = 0.0f;
    *(t + 1) = 0.0f;
    *(t + 2) = 0.0f;
    *(t + 3) = 0.0f;
#endif

    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_rect(s32bits x1, s32bits y1, s32bits x2, s32bits y2, u8bits r, u8bits g, u8bits b, u8bits a)
{
    float   *f, *v, *c;//, *t;
    float   fr, fg, fb, fa;
    float   fx1, fy1, fx2, fy2;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
	
    // 頂點
    v = (float *)m_fDefaultVertex;
    fx1 = (float)x1;
    fy1 = (float)y1;
    fx2 = (float)x2;
    fy2 = (float)y2;
    *(v    ) = fx1;
    *(v + 1) = fy1;
    *(v + 2) = fx2;
    *(v + 3) = fy1;
    *(v + 4) = fx2;
    *(v + 5) = fy2;
    *(v + 6) = fx1;
    *(v + 7) = fy2;
    // 顏色
    c = (float *)m_fDefaultColor;
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
    *(c + 4) = fr;
    *(c + 5) = fg;
    *(c + 6) = fb;
    *(c + 7) = fa;
    *(c + 8) = fr;
    *(c + 9) = fg;
    *(c +10) = fb;
    *(c +11) = fa;
    *(c +12) = fr;
    *(c +13) = fg;
    *(c +14) = fb;
    *(c +15) = fa;   
	// 貼圖座標
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = (float *)m_fDefaultTexCoord;
    *(t    ) = 0.0f;
    *(t + 1) = 0.0f;
    *(t + 2) = 0.0f;
    *(t + 3) = 0.0f;
    *(t + 4) = 0.0f;
    *(t + 5) = 0.0f;
    *(t + 6) = 0.0f;
    *(t + 7) = 0.0f;
#endif

    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_rect_fill(s32bits x1, s32bits y1, s32bits x2, s32bits y2, u8bits r, u8bits g, u8bits b, u8bits a)
{
    float   *f, *v, *c;//, *t;
    float   fr, fg, fb, fa;
    float   fx1, fy1, fx2, fy2;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
    
	// 頂點
    v = (float *)m_fDefaultVertex;
    fx1 = (float)x1;
    fy1 = (float)y1;
    fx2 = (float)x2;
    fy2 = (float)y2;
    *(v    ) = fx1;
    *(v + 1) = fy1;
    *(v + 2) = fx2;
    *(v + 3) = fy1;
    *(v + 4) = fx1;
    *(v + 5) = fy2;
    *(v + 6) = fx2;
    *(v + 7) = fy2;
    // 顏色
    c = (float *)m_fDefaultColor;
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
    *(c + 4) = fr;
    *(c + 5) = fg;
    *(c + 6) = fb;
    *(c + 7) = fa;
    *(c + 8) = fr;
    *(c + 9) = fg;
    *(c +10) = fb;
    *(c +11) = fa;
    *(c +12) = fr;
    *(c +13) = fg;
    *(c +14) = fb;
    *(c +15) = fa;    
	// 貼圖座標
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = (float *)m_fDefaultTexCoord;
    *(t    ) = 0.0f;
    *(t + 1) = 0.0f;
    *(t + 2) = 0.0f;
    *(t + 3) = 0.0f;
    *(t + 4) = 0.0f;
    *(t + 5) = 0.0f;
    *(t + 6) = 0.0f;
    *(t + 7) = 0.0f;
#endif

    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);  
	
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_circle(s32bits xc, s32bits yc, s32bits rc, u8bits r, u8bits g, u8bits b, u8bits a)
{
    u32bits angle;
    float   sx, sy, distance;
    float   *f, *v, *c;//, *t;
    float   si, cs;
    float   fr, fg, fb, fa;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
    
	// 中心點
    sx = (float)xc;
    sy = (float)yc;
	// 半徑
    distance = (float)rc;
    
    v = m_fDefaultVertex;
    c = m_fDefaultColor;
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = m_fDefaultTexCoord;
#endif    
    for(angle=0; angle<=360; angle++)
    {
        si = *(g_fSinTable + angle);
        cs = *(g_fCosTable + angle);
        // 頂點
        *(v    ) = sx + cs * distance;
        *(v + 1) = sy + si * distance;
        v += 2;
        // 顏色
        *(c    ) = fr;
        *(c + 1) = fg;
        *(c + 2) = fb;
        *(c + 3) = fa;
        c += 4;
		// 貼圖座標
#if 0      
        *(t    ) = 0.0f;
        *(t + 1) = 0.0f;
        t += 2;
#endif
    }
    
    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_LINE_STRIP, 0, 361);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_circle_fill(s32bits xc, s32bits yc, s32bits rc, u8bits r, u8bits g, u8bits b, u8bits a)
{
    u32bits angle;
    float   sx, sy, distance;
    float   *f, *v, *c;//, *t;
    float   si, cs;
    float   fr, fg, fb, fa;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
    
	// 中心點
    sx = (float)xc;
    sy = (float)yc;
	// 半徑
    distance = (float)rc;
	
    // 頂點 
    v = m_fDefaultVertex;
    *(v    ) = sx;
    *(v + 1) = sy;
    v += 2;
    // 顏色
    c = m_fDefaultColor;
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
    c += 4;
	// 貼圖座標
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = m_fDefaultTexCoord;
    *(t    ) = 0.0f;
    *(t + 1) = 0.0f;
    t += 2;
#endif    
    
    for(angle=0; angle<=360; angle++)
    {
        si = *(g_fSinTable + angle);
        cs = *(g_fCosTable + angle);
        // 頂點
        *(v    ) = sx + cs * distance;
        *(v + 1) = sy + si * distance;
        v += 2;
        // 顏色
        *(c    ) = fr;
        *(c + 1) = fg;
        *(c + 2) = fb;
        *(c + 3) = fa;
        c += 4;
		// 貼圖座標
 #if 0      
        *(t    ) = 0.0f;
        *(t + 1) = 0.0f;
        t += 2;
#endif
    }
    
    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 362);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_ellipse(s32bits xc, s32bits yc, s32bits ra, s32bits rb, u8bits r, u8bits g, u8bits b, u8bits a)
{
    u32bits angle;
    float   sx, sy, dx, dy;
    float   *f, *v, *c;//, *t;
    float   si, cs;
    float   fr, fg, fb, fa;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
    
	// 中心點
    sx = (float)xc;
    sy = (float)yc;
	// 半徑
    dx = (float)ra;
    dy = (float)rb;
    
    v = m_fDefaultVertex;
    c = m_fDefaultColor;
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = m_fDefaultTexCoord;
#endif    
    for(angle=0; angle<=360; angle++)
    {
        si = *(g_fSinTable + angle);
        cs = *(g_fCosTable + angle);        
         // 頂點
       *(v    ) = sx + cs * dx;
        *(v + 1) = sy + si * dy;
        v += 2;
        // 顏色
        *(c    ) = fr;
        *(c + 1) = fg;
        *(c + 2) = fb;
        *(c + 3) = fa;
        c += 4;
		// 貼圖座標
#if 0		
        *(t    ) = 0.0f;
        *(t + 1) = 0.0f;
        t += 2;
#endif
    }
    
    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_LINE_STRIP, 0, 361);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_ellipse_fill(s32bits xc, s32bits yc, s32bits ra, s32bits rb, u8bits r, u8bits g, u8bits b, u8bits a)
{
    u32bits angle;
    float   sx, sy, dx, dy;
    float   *f, *v, *c;//, *t;
    float   si, cs;
    float   fr, fg, fb, fa;
    
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
    
	// 中心點
    sx = (float)xc;
    sy = (float)yc;
	// 半徑
    dx = (float)ra;
    dy = (float)rb;
    
	// 頂點
    v = m_fDefaultVertex;
    *(v    ) = sx;
    *(v + 1) = sy;
    v += 2;
    // 顏色
    c = m_fDefaultColor;
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
    c += 4;    
	// 貼圖座標
	m_pfCurrentTexCoord = (float *)m_fZeroTexCoord;
#if 0	
    t = m_fDefaultTexCoord;
    *(t    ) = 0.0f;
    *(t + 1) = 0.0f;
    t += 2;
#endif    
    for(angle=0; angle<=360; angle++)
    {
        si = *(g_fSinTable + angle);
        cs = *(g_fCosTable + angle);
        // 頂點
        *(v    ) = sx + cs * dx;
        *(v + 1) = sy + si * dy;
        v += 2;
        // 顏色
        *(c    ) = fr;
        *(c + 1) = fg;
        *(c + 2) = fb;
        *(c + 3) = fa;
        c += 4;
		// 貼圖座標
#if 0        
        *(t    ) = 0.0f;
        *(t + 1) = 0.0f;
        t += 2;
#endif
    }
    
    gl_shader2d_use_program(DEF_USE_PROGRAM_TYPE_DEFAULT_VERTEX_COLOR);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idDefaultTexture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 362);
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_texture(u32bits program_type, 
								 s32bits x, s32bits y, u32bits w, u32bits h, 
								 u8bits r, u8bits g, u8bits b, u8bits a,
								 bool h_mirror, bool v_mirror,
								 s32bits angle,
								 u32bits texID, float tl, float tu, float tr, float td)
{
    s32bits     tmp_angle;
    float       *f, *v, *c, *t;
    float       fr, fg, fb, fa;
    float       centerX, centerY;
    float       si, cs;
    float       x1, y1, x2, y2;
    float       xx, yy;
    
    
    if(a == 0) return;
    if(w == 0 || h == 0) return;    
	
    f = (float *)g_fByteToFloatTable;
    fr = *(f + r);
    fg = *(f + g);
    fb = *(f + b);
    fa = *(f + a);
   
    x1 = (float)(x);
    y1 = (float)(y);
    x2 = x1 + (float)(w);
    y2 = y1 + (float)(h);
    
	// 頂點
    v = m_fDefaultVertex;  
    tmp_angle = angle;
    if(tmp_angle)
    {
        centerX = (float)(x + (w >> 1));
        centerY = (float)(y + (h >> 1));
        
        while(tmp_angle >= 360) tmp_angle -= 360;
        while(tmp_angle < 0) tmp_angle += 360;
        
        si = g_fSinTable[tmp_angle];
        cs = g_fCosTable[tmp_angle];
        
        xx = x1 - centerX;
        yy = y1 - centerY;
        *(v + 4) = centerX + (xx * si) + (yy * cs);
        *(v + 5) = centerY + (yy * si) - (xx * cs);
        
        xx = x2 - centerX;
        yy = y1 - centerY;
        *(v    ) = centerX + (xx * si) + (yy * cs);
        *(v + 1) = centerY + (yy * si) - (xx * cs);
        
        xx = x1 - centerX;
        yy = y2 - centerY;
        *(v + 6) = centerX + (xx * si) + (yy * cs);
        *(v + 7) = centerY + (yy * si) - (xx * cs);
        
        xx = x2 - centerX;
        yy = y2 - centerY;
        *(v + 2) = centerX + (xx * si) + (yy * cs);
        *(v + 3) = centerY + (yy * si) - (xx * cs);
    }
    else
    {
        /*
         0  1
         2  3
         */ 
        //     X              Y      
        *(v    ) = x1; *(v + 1) = y1; // 0 
        *(v + 2) = x2; *(v + 3) = y1; // 1 
        *(v + 4) = x1; *(v + 5) = y2; // 2 
        *(v + 6) = x2; *(v + 7) = y2; // 3 
    }
	
    // 顏色
    c = m_fDefaultColor;
    // 0 
    *(c    ) = fr;
    *(c + 1) = fg;
    *(c + 2) = fb;
    *(c + 3) = fa;
    // 1 
    *(c + 4) = fr;
    *(c + 5) = fg;
    *(c + 6) = fb;
    *(c + 7) = fa;
    // 2 
    *(c + 8) = fr;
    *(c + 9) = fg;
    *(c +10) = fb;
    *(c +11) = fa;
    // 3 
    *(c +12) = fr;
    *(c +13) = fg;
    *(c +14) = fb;
    *(c +15) = fa;
	
    // 貼圖座標
	m_pfCurrentTexCoord = (float *)m_fDefaultTexCoord;
    t = m_fDefaultTexCoord;
    if(h_mirror)
    {
        *(t    ) = tr; // u 1 
        *(t + 2) = tl; // u 0 
        *(t + 4) = tr; // u 1 
        *(t + 6) = tl; // u 0 
    }
    else
    {
        *(t    ) = tl; // u 0 
        *(t + 2) = tr; // u 1 
        *(t + 4) = tl; // u 0 
        *(t + 6) = tr; // u 1 
    }
    if(v_mirror)
    {
        *(t + 1) = td; // v 1 
        *(t + 3) = td; // v 1 
        *(t + 5) = tu; // v 0 
        *(t + 7) = tu; // v 0 
    }
    else
    {
        *(t + 1) = tu; // v 0 
        *(t + 3) = tu; // v 0 
        *(t + 5) = td; // v 1 
        *(t + 7) = td; // v 1 
    }
    
    gl_shader2d_use_program(program_type);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
	
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
//-----------------------------------------------------------------------------
static  u32bits  current_color;
static  s32bits  current_x, current_y;
//-----------------------------------------------------------------------------
void    gl_shader2d_color_set(u32bits color)
{
    current_color = color; 
}
//-----------------------------------------------------------------------------
void    gl_shader2d_move_to(s32bits x, s32bits y)
{
    current_x = x; 
    current_y = y;    
}
//-----------------------------------------------------------------------------
void    gl_shader2d_draw_line_to(s32bits x, s32bits y)
{
    u8bits r, g, b, a;

    r = (current_color      ) & 0xFF;
    g = (current_color >>  8) & 0xFF;
    b = (current_color >> 16) & 0xFF;
    a = (current_color >> 24) & 0xFF;

    gl_shader2d_draw_line(current_x, current_y, x, y, r, g, b, a); 
    current_x = x; 
    current_y = y;    
}
//-----------------------------------------------------------------------------
static 	char 	*m_pShader2dErrorString;
//-----------------------------------------------------------------------------
static  bool 	gl_shader2d_check_extensions(void)
{
	GLint 	major_version;
	GLint 	minor_version;
	
	non_power_of_two = true;
	m_pShader2dErrorString = null;	
		
	// 取得版本號
	glGetIntegerv(GL_MAJOR_VERSION, &major_version);
	glGetIntegerv(GL_MINOR_VERSION, &minor_version);
	
	// 檢查版本號 3.1 以上, 不用檢查後面幾項延伸功能
	if(major_version >= 3 && minor_version >= 1)
	{
		return true;
	}
	
	// 檢查是否支持 非 2 的 n 次方 的貼圖
	non_power_of_two = glewGetExtension("GL_ARB_texture_non_power_of_two");
	
	// 檢查是否支持 DX1、DX3、DX5 (.DDS檔)的貼圖壓縮
	if(glewGetExtension("GL_EXT_texture_compression_s3tc") != GLEW_OK)
	{
		m_pShader2dErrorString = (char *)"Driver does not support texture compress.\n";
		return false;		
	}
	
	// 檢查是否支持 fbo
	if(glewGetExtension("GL_EXT_framebuffer_object") != GLEW_OK) 
	{
		m_pShader2dErrorString = (char *)"Driver does not support Framebuffer Objects.\n";
		return false;
	}

	// 檢查是否支持 shader 語言
	if((glewGetExtension("GL_ARB_fragment_shader") && glewGetExtension("GL_ARB_vertex_shader") &&
	    glewGetExtension("GL_ARB_shader_objects") && glewGetExtension("GL_ARB_shading_language_100")) != GLEW_OK)
	{
		m_pShader2dErrorString = (char *)"Driver does not support OpenGL Shading Language.\n";
		return false;
	}
	
	return true;
}
//-----------------------------------------------------------------------------
bool 	gl_shader2d_init(u32bits w, u32bits h)
{
	u32bits i;
	float 	*t;

	// glew 初始化
    if(glewInit() != GLEW_OK)
    {
		m_pShader2dErrorString = (char *)"glewInit fail.\n";
		return false;
	}

    // 檢查 OpenGL 延伸功能
    if(gl_shader2d_check_extensions() == false) { return false; }

	gl_shader2d_lookup_table_init(); // 查詢表初始化
	
    m_ViewWidth = w;
    m_ViewHeight = h;
	
	t = m_fZeroTexCoord;
	for(i=0; i<(2 * 4 * DEF_MAX_DRAW_ARRAY_2D); i++)
	{
		*(t + i) = 0.0f;
	}
	
	// 上下顛倒繪製
    m_bFlipY = false;
	
	// 現在繪圖座標
	current_color = 0;
	current_x = 0;
	current_y = 0;
	
	// shader 程式type
	m_UseProgramType = DEF_USE_PROGRAM_TYPE_NULL;

	gl_shader2d_ortho_matrix_init(w, h); // 正交投影矩陣初始化	
	
	gl_shader2d_default_texture_init();	// default 貼圖初始化
	
	 // ascii 貼圖初始化 (基本秀字用 ascii 字型)
    if(gl_shader2d_ascii_texture_init() == false) { return false; }
	
    // 一般 shader 程式初始化 
    if(gl_shader2d_program_default_init() == false) { return false; }	
    // 一般 shader 程式初始化 (頂點顏色運算)
    if(gl_shader2d_program_default_vertex_color_init() == false) { return false; }
	
	// 指定顏色 shader 程式初始化
    if(gl_shader2d_program_assign_color_init() == false) { return false; }

	// 負片 shader 程式初始化
    if(gl_shader2d_program_negative_color_init() == false) { return false; }
	
	// 灰階 shader 程式初始化
    if(gl_shader2d_program_gray_color_init() == false) { return false; }
	
	// 清畫面
    gl_shader2d_background_color_set(0x00, 0x00, 0x00);
    gl_shader2d_clear();
	
	return true;
}
//-----------------------------------------------------------------------------
void    gl_shader2d_end(void)
{
    gl_shader2d_program_gray_color_end(); // 灰階 shader 程式結束
	
    gl_shader2d_program_negative_color_end(); // 負片 shader 程式結束
	
    gl_shader2d_program_assign_color_end(); // 指定顏色 shader 程式結束
	
    gl_shader2d_program_default_vertex_color_end(); // 一般 shader 程式結束 (頂點顏色運算)
    gl_shader2d_program_default_end(); // 一般 shader 程式結束
	
	gl_shader2d_ascii_texture_end(); // ascii 字型貼圖結束
	
    gl_shader2d_default_texture_end(); // default 貼圖結束
}
//-----------------------------------------------------------------------------
#define GL_GLEXT_PROTOTYPES 1
#define _USE_MATH_DEFINES

#include <windows.h>
#include <gl\glew.h>    // GL extensions - I used https://sourceforge.net/projects/glew/files/glew/1.9.0/glew-1.9.0.zip/download
#include <glut.h>       // GL utilities  - I used http://user.xmission.com/~nate/glut/glut-3.7.6-bin.zip

#include <iostream>

#include "data\my_data.h"
#include <iosfwd>

// Based on http://stupomerantz.com/public/opengl/example-06/example-06.cc.html


#define VERTEX_DATA_TYPE_FLOAT 0
#define VERTEX_DATA_TYPE_SHORT 1
#define VERTEX_DATA_TYPE_BYTE  2
#define VERTEX_DATA_TYPE       VERTEX_DATA_TYPE_FLOAT

#define PRIMITIVE_TYPE_TRIANGLE_ARRAY 0
#define PRIMITIVE_TYPE_TRIANGLE_STRIP 1
#define PRIMITIVE_TYPE                PRIMITIVE_TYPE_TRIANGLE_ARRAY

#define INDEXED_MODE_OFF 0
#define INDEXED_MODE_ON  1      // <- there's a bug with indexed mode ! TODO: fix
#define INDEXED_MODE     INDEXED_MODE_OFF


struct State 
{
    GLhandleARB shaderProgram;
    GLuint      vertexBufferID;

    GLint       positionLocation;  
    GLint       colorLocation;  

#if INDEXED_MODE==INDEXED_MODE_OFF
    int         vertexCount;
#endif

#if INDEXED_MODE==INDEXED_MODE_ON
    GLuint      indexBufferID;
    int         indexCount;
#endif
};
State g_State;

GLchar g_VertexShaderSource[] = "                                            \n\
#version 150                                                                 \n\
                                                                             \n\
in vec4 position;   // vertex attribute                                      \n\
in vec4 color;      // vertex attribute                                      \n\
                                                                             \n\
out vec4 mycolor;   // varying                                               \n\
                                                                             \n\
void main()                                                                  \n\
{                                                                            \n\
    gl_Position = position;                                                  \n\
    mycolor = color;                                                         \n\
}                                                                            \n\
";

GLchar g_FragmentShaderSource[] = "                                          \n\
#version 150                                                                 \n\
                                                                             \n\
in vec4 mycolor;    // varying                                               \n\
                                                                             \n\
out vec4 fragmentColor;                                                      \n\
                                                                             \n\
void main()                                                                  \n\
{                                                                            \n\
    fragmentColor = mycolor;                                                 \n\
}                                                                            \n\
";

GLhandleARB
shaderCompile(const GLchar *text, GLenum type)
{
    GLhandleARB shaderHandle;
    GLint       status;
    GLint       loglen;
    GLint       maxloglen;
    GLchar     *log = 0;
   
   
   shaderHandle = glCreateShader(type);

    if (shaderHandle == 0) {
        fprintf(stderr,"glCreateShader() failed.\n");
        fprintf(stderr,"Possibly outside an OpenGL context. Possible out of resources.\n");
        exit(1);
    }

    glShaderSource(shaderHandle, 1, &text, NULL);
    glutReportErrors();

    glCompileShader(shaderHandle);
    glutReportErrors();

    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &status);

    if (status != 0) {
        return shaderHandle;
    }

    // log length includes the c string null terminator
    glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maxloglen);
    log = new GLchar[maxloglen];

    glGetShaderInfoLog(shaderHandle, maxloglen, &loglen, log);

    fprintf(stderr,"compile failed.\n");
    fprintf(stderr,"shader text:\n");
    fprintf(stderr,"------------\n");
    fprintf(stderr,"%s\n",text);
    fprintf(stderr,"log text:\n");
    fprintf(stderr,"------------\n");
    fprintf(stderr,"%s\n",log);
    
    delete [] log;
    exit(1);
}

GLhandleARB
shaderProgramBuild(const GLchar *vertex, const GLchar *fragment)
{
    GLhandleARB programHandle;
    GLint       status;
    GLint       loglen;
    GLint       maxloglen;
    GLchar     *log = 0;

    programHandle = glCreateProgram();
    
    if (programHandle < 1) {
        fprintf(stderr,"glCreateShader() failed.\n");
        exit(1);
    }

    glutReportErrors();

    glAttachShader(programHandle, shaderCompile(vertex,GL_VERTEX_SHADER));
    glAttachShader(programHandle, shaderCompile(fragment,GL_FRAGMENT_SHADER));
    glutReportErrors();

    glLinkProgram(programHandle);
    glutReportErrors();

    glGetProgramiv(programHandle, GL_LINK_STATUS, &status);

    if (status != 0) {
        return programHandle;
    }

    glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &maxloglen);
    log = new GLchar[maxloglen];

    glGetProgramInfoLog(programHandle, maxloglen, &loglen, log);

    fprintf(stderr,"link failed.\n");
    fprintf(stderr,"log text:\n");
    fprintf(stderr,"------------\n");
    fprintf(stderr,"%s\n",log);
    
    delete [] log;
    exit(1);
}




void loadData()
{
    FILE* f = fopen("data\\my_data.bin", "rb");

    if(!f)
    {
        fprintf(stderr,"data file not found\n");
        exit(1);
    }

    // get file size
    fseek(f, 0, SEEK_END);
    long dataByteSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    int vertexDataItemSize = sizeof(VertexData);

    int vertexDataSize = g_VertexCount * vertexDataItemSize;
    VertexData* vertexData = new VertexData[g_VertexCount];
    fread(vertexData, vertexDataItemSize, g_VertexCount, f);

    int indexDataSize  = dataByteSize - vertexDataSize;
#if INDEXED_MODE==INDEXED_MODE_OFF
    if( indexDataSize != 0 )
    {
        fprintf(stderr, "data size error \n");
        __debugbreak();
    }
#endif

#if INDEXED_MODE==INDEXED_MODE_ON
    int indexItemSize = sizeof(int);
    g_State.indexCount = indexDataSize / indexItemSize;
     
    int* indexData = new int[g_State.indexCount];
    fread(indexData, indexItemSize, g_State.indexCount, f);
#endif

    fclose(f);
    

    glGenBuffers(1, &g_State.vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, g_State.vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, GL_STATIC_DRAW);
    delete [] vertexData;

#if INDEXED_MODE==INDEXED_MODE_ON
    glGenBuffers(1, &g_State.indexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, g_State.indexBufferID);
    glBufferData(GL_ARRAY_BUFFER, indexDataSize, indexData, GL_STATIC_DRAW);
    delete [] indexData;
#endif
}






void
initialize()
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f) ;

    g_State.shaderProgram = shaderProgramBuild(g_VertexShaderSource, g_FragmentShaderSource);

    glUseProgram(g_State.shaderProgram);
    g_State.positionLocation = glGetAttribLocation(g_State.shaderProgram, "position");
    // get the location of the color shader input
    g_State.colorLocation  = glGetAttribLocation(g_State.shaderProgram, "color");    
    glUseProgram(0);

    // make sure we were able to successfully get the location.
    if (g_State.positionLocation < 0) {
        fprintf(stderr,"unable to get the location of 'position'\n");
        exit(1);
    }

    loadData();

    glFinish();
}

void 
display() 
{
    static bool doInitialize = true;

    if (doInitialize == true) {
        doInitialize = false;
        initialize();
    }

	glClear(GL_COLOR_BUFFER_BIT) ;

    glUseProgram(g_State.shaderProgram);

    // bind the vertex buffer for drawing
    glBindBuffer(GL_ARRAY_BUFFER, g_State.vertexBufferID); 
    //glEnableClientState(GL_VERTEX_ARRAY);

#if INDEXED_MODE==INDEXED_MODE_ON
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_State.indexBufferID); 
#endif

    int attributesCount    = 2;         // position and color
    GLint     components   = 4;         // both position and color have 4 components (x, y, z, w)

#if VERTEX_DATA_TYPE==VERTEX_DATA_TYPE_FLOAT
    // For floats
    GLenum    type         = GL_FLOAT;
    GLboolean normalized   = GL_FALSE;
    GLint     dataTypeSize = sizeof(GLfloat);
#endif

#if VERTEX_DATA_TYPE==VERTEX_DATA_TYPE_SHORT
    // For shorts
    GLenum    type         = GL_SHORT;
    GLboolean normalized   = GL_TRUE;
    GLint     dataTypeSize = sizeof(GLshort);
#endif

#if VERTEX_DATA_TYPE==VERTEX_DATA_TYPE_BYTE
    // For bytes
    GLenum    type         = GL_BYTE;
    GLboolean normalized   = GL_TRUE;
    GLint     dataTypeSize = sizeof(GLbyte);
#endif

    GLsizei   stride       = attributesCount * components * dataTypeSize;
    GLvoid*   positionArrayStart = 0;
    GLint     colorArrayStart = components * dataTypeSize;   // first color data is after the first position data
    glVertexAttribPointer(g_State.positionLocation, components, type, normalized, stride, positionArrayStart);
    glVertexAttribPointer(g_State.colorLocation,    components, type, normalized, stride, (GLvoid*)colorArrayStart);
       
    // enable both buffers
    glEnableVertexAttribArray(g_State.colorLocation);
    glEnableVertexAttribArray(g_State.positionLocation);

    // DRAW
#if PRIMITIVE_TYPE==PRIMITIVE_TYPE_TRIANGLE_ARRAY
    #if INDEXED_MODE==INDEXED_MODE_ON
        glDrawArrays(GL_TRIANGLES, 0, g_State.indexCount);
        //glDrawElements(GL_TRIANGLES, g_State.indexCount, GL_UNSIGNED_INT, )
    #else
        glDrawArrays(GL_TRIANGLES, 0, g_VertexCount);
    #endif
#endif

#if PRIMITIVE_TYPE==PRIMITIVE_TYPE_TRIANGLE_STRIP
    #if INDEXED_MODE==INDEXED_MODE_ON
        glDrawArrays(GL_TRIANGLE_STRIP, 0, g_State.indexCount);  
    #else
        glDrawArrays(GL_TRIANGLE_STRIP, 0, g_VertexCount);  
    #endif
#endif

    glUseProgram(0);

    glFlush();

	glutReportErrors();

	return ;
}

void
reshape(int width, int height)
{
    glViewport(0, 0, width, height);
}

int main(int argc, char **argv) 
{
	glutInit( &argc, argv);
	glutInitWindowSize(800,600);
	glutInitDisplayMode(GLUT_RGB|GLUT_SINGLE);

	glutCreateWindow("VBO-example");

	glutDisplayFunc( display );
    glutReshapeFunc( reshape );

    GLint GlewInitResult = glewInit();
    if (GLEW_OK != GlewInitResult) 
    {
        printf("ERROR: %s\n",glewGetErrorString(GlewInitResult));
        exit(EXIT_FAILURE);
    }

	glutMainLoop() ;	

	return(0) ;
}




#define GL_GLEXT_PROTOTYPES 1
#define _USE_MATH_DEFINES

#include <windows.h>
#include <gl\glew.h>    // GL extensions - I used https://sourceforge.net/projects/glew/files/glew/1.9.0/glew-1.9.0.zip/download
#include <glut.h>       // GL utilities  - I used http://user.xmission.com/~nate/glut/glut-3.7.6-bin.zip

#include <iostream>

#include "data\my_data.h"

// Based on http://stupomerantz.com/public/opengl/example-06/example-06.cc.html

struct State 
{
    GLhandleARB shaderProgram;
    GLuint      dataBufferID;
    GLint       positionLocation;  
    GLint       colorLocation;

    int vertexCount;
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



// return vertex count
int loadVertexData()
{
    FILE* f = fopen("data\\my_data.bin", "r");

    if(!f)
    {
        fprintf(stderr,"data file not found\n");
        exit(1);
    }

    // get file size
    fseek(f, 0, SEEK_END);
    long dataByteSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    int itemSize = sizeof(VertexData);
    int vertexCount = dataByteSize / itemSize;

    VertexData* vertexData = new VertexData[vertexCount];
    fread(vertexData, itemSize, vertexCount, f);
    fclose(f);

    glGenBuffers(1, &g_State.dataBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, g_State.dataBufferID);
    glBufferData(GL_ARRAY_BUFFER, dataByteSize, vertexData, GL_STATIC_DRAW);
    
    delete [] vertexData;

    return vertexCount;
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

    g_State.vertexCount = loadVertexData();

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
    glBindBuffer(GL_ARRAY_BUFFER, g_State.dataBufferID);
    

    int attributesCount = 2;            // position and color
    GLint     components = 4;           // both position and color have 4 components
    GLenum    type       = GL_FLOAT;
    GLboolean normalized = GL_FALSE;
    GLsizei   stride     = attributesCount * components * sizeof(GLfloat);
    GLvoid*   positionArrayStart = 0;
    GLint     colorArrayStart = components * sizeof(GLfloat);   // first color data is after the first position data
    glVertexAttribPointer(g_State.positionLocation, components, type, normalized, stride, positionArrayStart);
    glVertexAttribPointer(g_State.colorLocation,    components, type, normalized, stride, (GLvoid*)colorArrayStart);
       
    // enable both buffers
    glEnableVertexAttribArray(g_State.colorLocation);
    glEnableVertexAttribArray(g_State.positionLocation);

        // DRAW
        glDrawArrays(GL_TRIANGLES, 0, g_State.vertexCount);

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



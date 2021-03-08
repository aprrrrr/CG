/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: aprilc
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

GLuint vboSolid, vaoSolid;
GLuint vboWireframe, vaoWireframe;
GLuint vboPoint, vaoPoint;
GLuint vboSmooth, vboSmoothUp, vboSmoothDown, vboSmoothLeft, vboSmoothRight, vaoSmooth;

size_t numVertices = 0;

OpenGLMatrix matrix; //openGLMatrix
BasicPipelineProgram * pipelineProgram;

GLint h_modelViewMatrix, h_projectionMatrix;

glm::vec4 color_white(1, 1, 1, 1);

int renderMode = 1;
float heightScale = 0.25;

std::vector<glm::vec3> positions;
std::vector<glm::vec4> colors;
std::vector<glm::vec3> positions_line;
std::vector<glm::vec4> colors_line;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(128, 300, 128, 128, 0, 128, 0, 0, 1);
 
  // Translate, Rotate, Scale
  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix.Rotate(landRotate[0], 1, 0, 0);
  matrix.Rotate(landRotate[1], 0, 1, 0);
  matrix.Rotate(landRotate[2], 0, 0, 1);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);

  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);

  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);

  // get a handle to the program
  GLuint program = pipelineProgram->GetProgramHandle();
  // get a handle to the modelViewMatrix shader variable
	h_modelViewMatrix =
	  glGetUniformLocation(program, "modelViewMatrix");
  // upload m to the GPU
  pipelineProgram->Bind(); // must do (once) before glUniformMatrix4fv
  GLboolean isRowMajor = GL_FALSE;
  glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
  h_projectionMatrix =
	  glGetUniformLocation(program, "projectionMatrix");
  // upload p to the GPU
  glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);

  // 
  // bind shader
  pipelineProgram->Bind();

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);

  // bind the VAO
  switch (renderMode)
  {
  case 1:
	  glBindVertexArray(vaoPoint);
	  glDrawArrays(GL_POINTS, 0, numVertices);
	  break;
  case 2:
	  glBindVertexArray(vaoWireframe);
	  glDrawArrays(GL_LINES, 0, positions_line.size());
	  break;
  }


  // unbind the VAO
  glBindVertexArray(0);

  glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff... 

  // for example, here, you can save the screenshots to disk (to make the animation)

  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 1000.0f);

  // set the mode back to ModelView
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
	
	case '1':
		cout << "You pressed 1" << endl;
		renderMode = 1;
	break;

	case '2':
		cout << "You pressed 2" << endl;
		renderMode = 2;
	break;

	case '3':
		cout << "You pressed 3" << endl;
		renderMode = 3;
	break;

	case '4':
		cout << "You pressed 4" << endl;
		renderMode = 4;
	break;
  }
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // background color
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // read in pixels
  int imageWidth = heightmapImage->getWidth();
  int imageHeight = heightmapImage->getHeight();

  for (int i = 0; i < imageWidth; i++)
  {
	  for (int j = 0; j < imageHeight; j++)
	  {
		  float height = heightScale * heightmapImage->getPixel(i, j, 0);
		  glm::vec3 position = glm::vec3(i, height, -j);

		  // point
		  positions.push_back(position);
		  colors.push_back(color_white);

		  // wireframe
		  if (i != 0)
		  {
			  positions_line.push_back(position);
			  positions_line.push_back(positions[numVertices - imageHeight]);
			  colors_line.push_back(color_white);
			  colors_line.push_back(color_white);
		  }
		  if (j != 0)
		  {
			  positions_line.push_back(position);
			  positions_line.push_back(positions[numVertices - 1]);
			  colors_line.push_back(color_white);
			  colors_line.push_back(color_white);
		  }

		  numVertices++;
	  }
  }
  

  // point
  size_t positionSize = sizeof(glm::vec3) * numVertices;
  size_t colorSize = sizeof(glm::vec4) * numVertices;
  glGenBuffers(1, &vboPoint);
  glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
  glBufferData(GL_ARRAY_BUFFER, positionSize + colorSize, nullptr,
               GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize, &positions[0]);
  glBufferSubData(GL_ARRAY_BUFFER, positionSize, colorSize, &colors[0]);

  // wireframe
  size_t positionSize_line = sizeof(glm::vec3) * positions_line.size();
  size_t colorSize_line = sizeof(glm::vec4) * colors_line.size();
  glGenBuffers(1, &vboWireframe);
  glBindBuffer(GL_ARRAY_BUFFER, vboWireframe);
  glBufferData(GL_ARRAY_BUFFER, positionSize_line + colorSize_line, nullptr,
	  GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize_line, &positions_line[0]);
  glBufferSubData(GL_ARRAY_BUFFER, positionSize_line, colorSize_line, &colors_line[0]);

  //
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  // point
  glGenVertexArrays(1, &vaoPoint);
  glBindVertexArray(vaoPoint);
  glBindBuffer(GL_ARRAY_BUFFER, vboPoint);

  GLuint loc =
      glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)positionSize);

  // wireframe
  glGenVertexArrays(1, &vaoWireframe);
  glBindVertexArray(vaoWireframe);
  glBindBuffer(GL_ARRAY_BUFFER, vboWireframe);

  loc =
	  glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)positionSize_line);


  glEnable(GL_DEPTH_TEST);


  // initialize modelview and projection matrix
  GLuint program = pipelineProgram->GetProgramHandle();
  h_modelViewMatrix =
	  glGetUniformLocation(program, "modelViewMatrix");
  h_projectionMatrix =	
	  glGetUniformLocation(program, "projectionMatrix");

  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}



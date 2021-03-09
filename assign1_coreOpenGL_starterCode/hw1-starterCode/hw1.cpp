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

// vbo & vao
GLuint vboSolid, vaoSolid;
GLuint vboWireframe, vaoWireframe;
GLuint vboPoint, vaoPoint;
GLuint vboSmooth, vboSmoothUp, vboSmoothDown, vboSmoothLeft, vboSmoothRight, vaoSmooth;

size_t numVertices = 0;

OpenGLMatrix matrix; 
BasicPipelineProgram * pipelineProgram;

glm::vec4 color_white(1, 1, 1, 1);

int renderMode = 1;
float heightScale = 0.1;

std::vector<glm::vec4> colors, colors_point, colors_line, colors_triangle;
std::vector<glm::vec3> positions, positions_line, positions_triangle, 
	positions_left, positions_right, positions_down, positions_up,
	positions_tri_left, positions_tri_right, positions_tri_up, positions_tri_down;

int numScreenshots = 0;
bool startRecord = false;

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
  matrix.Rotate(landRotate[2], 0, 0, -1);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);

  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);

  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);

  // bind shader
  pipelineProgram->Bind();

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);
  int mode = renderMode == 4 ? 1 : 0;
  pipelineProgram->SetRenderMode(mode);

  // bind the VAO
  switch (renderMode)
  {
	  // point mode
  case 1:
	  glBindVertexArray(vaoPoint);
	  glDrawArrays(GL_POINTS, 0, numVertices);
	  break;
	  // wireframe mode
  case 2:
	  glBindVertexArray(vaoWireframe);
	  glDrawArrays(GL_LINES, 0, positions_line.size());
	  break;
  case 3:
	  // solid mode
	  glBindVertexArray(vaoSolid);
	  glDrawArrays(GL_TRIANGLES, 0, positions_triangle.size());
	  break;
  case 4:
	  // smooth mode
	  glBindVertexArray(vaoSmooth);
	  glDrawArrays(GL_TRIANGLES, 0, positions_triangle.size());
	  break;
  }

  // unbind the VAO
  glBindVertexArray(0);

  glutSwapBuffers();
}

void idleFunc()
{
  // when user hits 'a', starts taking screenshots until there are 300 
	if (startRecord && numScreenshots <= 300)
	{
		char fileName[5];
		sprintf(fileName, "%03d", numScreenshots);
		saveScreenshot(("animation/" + std::string(fileName) + ".jpg").c_str());
		numScreenshots++;
	}
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
		cout << "Switch to Point mode." << endl;
		renderMode = 1;
	break;

	case '2':
		cout << "Switch to Wireframe mode." << endl;
		renderMode = 2;
	break;

	case '3':
		cout << "Switch to Solid mode." << endl;
		renderMode = 3;
	break;

	case '4':
		cout << "Switch to Smooth mode." << endl;
		renderMode = 4;
	break;
	case 'a':
		cout << "You pressed A. Start recording..." << endl;
		startRecord = true;
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

  // read in pixels from image
  int imageWidth = heightmapImage->getWidth();
  int imageHeight = heightmapImage->getHeight();

  for (int i = 0; i < imageWidth; i++)
  {
	  for (int j = 0; j < imageHeight; j++)
	  {
		  // get greyscale value
		  float grayscale = heightmapImage->getPixel(i, j, 0);
		  float height = heightScale * grayscale;
		  grayscale /= 255;
		  // convert to position for vertices
		  glm::vec3 position = glm::vec3(i, height, -j);
		  // convert to rgb
		  glm::vec4 color(grayscale, grayscale, grayscale, 1);
		  // fill vector of colors
		  colors.push_back(color);

		  // fill positions and colors_point for point mode
		  positions.push_back(position);
		  colors_point.push_back(color_white);

		  // fill positions_line and colors_line for wireframe mode
		  if (i != 0)
		  {
			  positions_line.push_back(position); // current vertex
			  positions_line.push_back(positions[numVertices - imageHeight]); // vertex below
			  colors_line.push_back(color_white);
			  colors_line.push_back(color_white);
		  }
		  if (j != 0)
		  {
			  positions_line.push_back(position); // current vertex
			  positions_line.push_back(positions[numVertices - 1]); // vertex to the left
			  colors_line.push_back(color_white);
			  colors_line.push_back(color_white);
		  }
		  numVertices++;
	  }
  }

  // calculate corresponding positions of surrounding vertices
  numVertices = 0;
  for (int i = 0; i < imageWidth; i++)
  {
	  for (int j = 0; j < imageHeight; j++)
	  {
		  glm::vec3 posCenter, posLeft, posRight, posUp, posDown;
		  posCenter = positions[numVertices];
		  posLeft = posRight = posUp = posDown = posCenter; // set to posCenter when not applicable
		  if (i != 0) posDown = positions[numVertices - imageHeight];
		  if (i != imageWidth - 1) posUp = positions[numVertices + imageHeight];
		  if (j != 0) posLeft = positions[numVertices - 1];
		  if (j != imageHeight - 1) posRight = positions[numVertices + 1];
		  positions_down.push_back(posDown);
		  positions_up.push_back(posUp);
		  positions_left.push_back(posLeft);
		  positions_right.push_back(posRight);
		  numVertices++;
	  }
  }

  // fill positions_triangle, colors_triangle for Solid mode
  // fill positions_tri_left/right/up/down for smoothing
  numVertices = 0;
  for (int i = 0; i < imageWidth; i++)
  {
	  for (int j = 0; j < imageHeight; j++)
	  {
		  if (i != 0 && j != 0)
		  {
			  glm::vec3 position = positions[numVertices];
			  glm::vec4 color = colors[numVertices];

			  glm::vec3 right = positions_right[numVertices];
			  glm::vec3 up = positions_up[numVertices];

			  glm::vec3 left = positions[numVertices - 1];
			  glm::vec3 left_left = positions_left[numVertices - 1];
			  glm::vec3 left_right = positions_right[numVertices - 1];
			  glm::vec3 left_up = positions_up[numVertices - 1];
			  glm::vec3 left_down = positions_down[numVertices - 1];

			  glm::vec3 down = positions[numVertices - imageHeight];
			  glm::vec3 down_left = positions_left[numVertices - imageHeight];
			  glm::vec3 down_right = positions_right[numVertices - imageHeight];
			  glm::vec3 down_up = positions_up[numVertices - imageHeight];
			  glm::vec3 down_down = positions_down[numVertices - imageHeight];

			  glm::vec3 lowerleft = positions[numVertices - imageHeight - 1];
			  glm::vec3 lowerleft_left = positions_left[numVertices - imageHeight - 1];
			  glm::vec3 lowerleft_right = positions_right[numVertices - imageHeight - 1];
			  glm::vec3 lowerleft_up = positions_up[numVertices - imageHeight - 1];
			  glm::vec3 lowerleft_down = positions_down[numVertices - imageHeight - 1];

			  glm::vec4 leftColor = colors[numVertices - 1];
			  glm::vec4 downColor = colors[numVertices - imageHeight];
			  glm::vec4 lowerleftColor = colors[numVertices - imageHeight - 1];

			  positions_triangle.push_back(position);
			  positions_triangle.push_back(left);
			  positions_triangle.push_back(lowerleft);

			  positions_tri_left.push_back(left);
			  positions_tri_right.push_back(right);
			  positions_tri_up.push_back(up);
			  positions_tri_down.push_back(down);

			  positions_tri_left.push_back(left_left);
			  positions_tri_right.push_back(left_right);
			  positions_tri_up.push_back(left_up);
			  positions_tri_down.push_back(left_down);

			  positions_tri_left.push_back(lowerleft_left);
			  positions_tri_right.push_back(lowerleft_right);
			  positions_tri_up.push_back(lowerleft_up);
			  positions_tri_down.push_back(lowerleft_down);

			  colors_triangle.push_back(color);
			  colors_triangle.push_back(leftColor);
			  colors_triangle.push_back(lowerleftColor);


			  positions_triangle.push_back(position);
			  positions_triangle.push_back(lowerleft);
			  positions_triangle.push_back(down);

			  colors_triangle.push_back(color);
			  colors_triangle.push_back(lowerleftColor);
			  colors_triangle.push_back(downColor);

			  positions_tri_left.push_back(left);
			  positions_tri_right.push_back(right);
			  positions_tri_up.push_back(up);
			  positions_tri_down.push_back(down);

			  positions_tri_left.push_back(lowerleft_left);
			  positions_tri_right.push_back(lowerleft_right);
			  positions_tri_up.push_back(lowerleft_up);
			  positions_tri_down.push_back(lowerleft_down);

			  positions_tri_left.push_back(down_left);
			  positions_tri_right.push_back(down_right);
			  positions_tri_up.push_back(down_up);
			  positions_tri_down.push_back(down_down);
		  }
		  numVertices++;
	  }
  }

  // upload data for point mode
  size_t positionSize = sizeof(glm::vec3) * numVertices;
  size_t colorSize = sizeof(glm::vec4) * numVertices;
  glGenBuffers(1, &vboPoint);
  glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
  glBufferData(GL_ARRAY_BUFFER, positionSize + colorSize, nullptr,
               GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize, &positions[0]);
  glBufferSubData(GL_ARRAY_BUFFER, positionSize, colorSize, &colors_point[0]);

  // upload data for wireframe mode
  size_t positionSize_line = sizeof(glm::vec3) * positions_line.size();
  size_t colorSize_line = sizeof(glm::vec4) * colors_line.size();
  glGenBuffers(1, &vboWireframe);
  glBindBuffer(GL_ARRAY_BUFFER, vboWireframe);
  glBufferData(GL_ARRAY_BUFFER, positionSize_line + colorSize_line, nullptr,
	  GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize_line, &positions_line[0]);
  glBufferSubData(GL_ARRAY_BUFFER, positionSize_line, colorSize_line, &colors_line[0]);

  // upload data for solid mode
  size_t positionSize_triangle = sizeof(glm::vec3) * positions_triangle.size();
  size_t colorSize_triangle = sizeof(glm::vec4) * colors_triangle.size();
  glGenBuffers(1, &vboSolid);
  glBindBuffer(GL_ARRAY_BUFFER, vboSolid);
  glBufferData(GL_ARRAY_BUFFER, positionSize_triangle + colorSize_triangle, nullptr,
	  GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize_triangle, &positions_triangle[0]);
  glBufferSubData(GL_ARRAY_BUFFER, positionSize_triangle, colorSize_triangle, &colors_triangle[0]);

  // upload data for smooth mod
  //	center
  glGenBuffers(1, &vboSmooth);
  glBindBuffer(GL_ARRAY_BUFFER, vboSmooth);
  glBufferData(GL_ARRAY_BUFFER, positionSize_triangle + colorSize_triangle, nullptr,
	  GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize_triangle, &positions_triangle[0]);
  glBufferSubData(GL_ARRAY_BUFFER, positionSize_triangle, colorSize_triangle, &colors_triangle[0]);
  //	left
  glGenBuffers(1, &vboSmoothLeft);
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothLeft);
  glBufferData(GL_ARRAY_BUFFER, positionSize_triangle, &positions_tri_left[0], GL_STATIC_DRAW);
  //	right
  glGenBuffers(1, &vboSmoothRight);
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothRight);
  glBufferData(GL_ARRAY_BUFFER, positionSize_triangle, &positions_tri_right[0], GL_STATIC_DRAW);
  //	up
  glGenBuffers(1, &vboSmoothUp);
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothUp);
  glBufferData(GL_ARRAY_BUFFER, positionSize_triangle, &positions_tri_up[0], GL_STATIC_DRAW);
  //	down
  glGenBuffers(1, &vboSmoothDown);
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothDown);
  glBufferData(GL_ARRAY_BUFFER, positionSize_triangle, &positions_tri_down[0], GL_STATIC_DRAW);

  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  // bind vao for Point mode
  glGenVertexArrays(1, &vaoPoint);
  glBindVertexArray(vaoPoint);
  glBindBuffer(GL_ARRAY_BUFFER, vboPoint);

  GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)positionSize);

  // bind vao for Wireframe mode
  glGenVertexArrays(1, &vaoWireframe);
  glBindVertexArray(vaoWireframe);
  glBindBuffer(GL_ARRAY_BUFFER, vboWireframe);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)positionSize_line);

  // bind vao for Solid mode
  glGenVertexArrays(1, &vaoSolid);
  glBindVertexArray(vaoSolid);
  glBindBuffer(GL_ARRAY_BUFFER, vboSolid);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)positionSize_triangle);

  // bind vao for Smooth mode
  glGenVertexArrays(1, &vaoSmooth);
  glBindVertexArray(vaoSmooth);

  //	center
  glBindBuffer(GL_ARRAY_BUFFER, vboSmooth);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)positionSize_triangle);

  //	left
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothLeft);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "positionLeft");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  //	right
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothRight);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "positionRight");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  //	up
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothUp);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "positionUp");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  //	down
  glBindBuffer(GL_ARRAY_BUFFER, vboSmoothDown);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "positionDown");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);


  glEnable(GL_DEPTH_TEST);

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



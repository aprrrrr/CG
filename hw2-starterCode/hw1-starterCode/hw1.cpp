/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
char windowTitle[512] = "CSCI 420 homework II";

OpenGLMatrix matrix; 
BasicPipelineProgram * pipelineProgram;

int imageWidth, imageHeight;

int numScreenshots = 0;
bool startRecord = false;

GLuint vao, vbo;

glm::vec4 color_white(1, 1, 1, 1);
std::vector<glm::vec4> colors_line;
std::vector<glm::vec3> verts_line;
std::vector<glm::vec3> verts_tube;
std::vector<glm::vec4> colors_tube;
glm::mat4 basis;
glm::mat3x4 control;

std::vector<glm::vec3> tangents;
std::vector<glm::vec3> normals;
std::vector<glm::vec3> positions;

glm::vec3 eye(0.0f, 2.0f, 0.0f);
glm::vec3 center(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 0.0f, 1.0f);
int animationID = 0;

float a = 0.01f;


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

  // set camera
  matrix.LookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);
 
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

  // bind the VAO
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, verts_tube.size());

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

	// animate the ride
	eye = positions[animationID] + glm::vec3(0.0f,1.0f,0.0f);
	center = tangents[animationID];
	up = normals[animationID];
	animationID++;
	animationID %= positions.size();

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

	case 'a':
		cout << "You pressed A. Start recording..." << endl;
		startRecord = true;
		break;

  }
}

// represents one control point along the spline 
struct Point
{
	double x;
	double y;
	double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
	int numControlPoints;
	Point* points;
};

// the spline array 
Spline* splines;
// total number of splines 
int numSplines;

int loadSplines(char* argv)
{
	char* cName = (char*)malloc(128 * sizeof(char));
	FILE* fileList;
	FILE* fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL)
	{
		printf("can't open file\n");
		exit(1);
	}

	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*)malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++)
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL)
		{
			printf("can't open file\n");
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point*)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf",
			&splines[j].points[i].x,
			&splines[j].points[i].y,
			&splines[j].points[i].z) != EOF)
		{
			i++;
		}
	}

	free(cName);

	return 0;
}

int initTexture(const char* imageFilename, GLuint textureHandle)
{
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK)
	{
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4)
	{
		printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

	// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++)
		{
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

			// set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0)
	{
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}

	// de-allocate the pixel array -- it is no longer needed
	delete[] pixelsRGBA;

	return 0;
}

void initVBO()
{
	// upload data for point mode
	size_t positionSize = sizeof(glm::vec3) * verts_tube.size();
	//size_t positionSize = sizeof(glm::vec3) * verts_line.size();
	size_t colorSize = sizeof(glm::vec4) * colors_tube.size();
	//size_t colorSize = sizeof(glm::vec4) * colors_line.size();
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, positionSize + colorSize, nullptr,
		GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, positionSize, &verts_tube[0]);
	glBufferSubData(GL_ARRAY_BUFFER, positionSize, colorSize, &colors_tube[0]);

	// bind vao for Point mode
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)positionSize);
}

void generateVertices()
{
	float s = 0.5f;
	basis = glm::mat4( -s, 2 * s, -s, 0,
					2 - s, s - 3, 0, 1,
					s - 2, 3 - 2 * s, s, 0.0f,
					s, -s, 0.0f, 0.0f );

	Spline spline = splines[0];
	glm::vec3 p, b, n;
	glm::vec3 v[8]; // 8 vertices of tube's cross section

	for (int i = 0; i < spline.numControlPoints - 3; i++)
	{
		control = glm::mat3x4(spline.points[i].x, spline.points[i + 1].x, spline.points[i + 2].x, spline.points[i + 3].x, 
			spline.points[i].y, spline.points[i + 1].y, spline.points[i + 2].y, spline.points[i + 3].y,
			spline.points[i].z, spline.points[i + 1].z,	spline.points[i + 2].z, spline.points[i + 3].z);

		for (float u = 0.0f; u <= 1.0f; u += 0.001f)
		{
			// fill in vertices for line
			if (verts_line.size() > 1)
			{
				verts_line.push_back(p);
				colors_line.push_back(color_white);
			}

			// calculate position
			glm::vec4 uVec(pow(u, 3), pow(u, 2), u, 1);
			p = uVec * basis * control;

			// fill in vertices and colors for line
			verts_line.push_back(p);
			colors_line.push_back(color_white);

			// calculate tangent
			// t(u) = p'(u) = [3u^2 2u 1 0] M C
			glm::vec4 duVec(3 * pow(u, 2), 2 * u, 1, 0);
			glm::vec3 t = duVec * basis * control;
			t = glm::normalize(t);
			tangents.push_back(t);

			// calculate normal
			if (normals.empty())
			{
				// N0 = unit(T0 x V)
				n = cross(t, glm::vec3(0.0f, 0.0f, 1.0f));
				n = glm::normalize(n);
				// B0 = unit(T0 x N0)
				b = cross(t, n);
				b = glm::normalize(b);
			}
			else
			{
				// N1 = unit(B0 x T1) 
				n = cross(b, t);
				n = glm::normalize(n);
				// B1 = unit(T1 x N1)
				b = cross(t , n);
				b = glm::normalize(b);
			}
			normals.push_back(n);

			// fill in vertices and colors for tube
			if (positions.empty())
			{
				// calculate vertices around p0
				v[0] = p + a * (-n + b);
				v[1] = p + a * (n + b);
				v[2] = p + a * (n - b);
				v[3] = p + a * (-n - b);
			}
			else
			{
				// calculate vertices around p1
				v[4] = p + a * (-n + b);
				v[5] = p + a * (n + b);
				v[6] = p + a * (n - b);
				v[7] = p + a * (-n - b);

				// add vertices of triangles
				// (0,4,1), (1,4,5), (1,5,2), (2,5,6), (2,6,3), (3,6,7), (3,7,0), (0,7,4)
				verts_tube.push_back(v[0]);//041
				verts_tube.push_back(v[4]);
				verts_tube.push_back(v[1]);
				verts_tube.push_back(v[1]);//145
				verts_tube.push_back(v[4]);
				verts_tube.push_back(v[5]);

				verts_tube.push_back(v[1]);//152
				verts_tube.push_back(v[5]);
				verts_tube.push_back(v[2]);
				verts_tube.push_back(v[2]);//256
				verts_tube.push_back(v[5]);
				verts_tube.push_back(v[6]);

				verts_tube.push_back(v[2]);//263
				verts_tube.push_back(v[6]);
				verts_tube.push_back(v[3]);
				verts_tube.push_back(v[3]);//367
				verts_tube.push_back(v[6]);
				verts_tube.push_back(v[7]);

				verts_tube.push_back(v[3]);//370
				verts_tube.push_back(v[7]);
				verts_tube.push_back(v[0]);
				verts_tube.push_back(v[0]);//074
				verts_tube.push_back(v[7]);
				verts_tube.push_back(v[4]);

				// add colors of triangles (rgb = normal)
				glm::vec3 color;
				for (int i = 0; i < 4; i++)
				{
					if (i == 0) color = b;
					else if (i == 1) color = n;
					else if (i == 2) color = -b;
					else color = -n;
					for (int j = 0; j < 6; j++)
						colors_tube.push_back(glm::vec4(color, 1));
				}

				// p0 = p1, copy v4-7 to v0-3
				v[0] = v[4];
				v[1] = v[5];
				v[2] = v[6];
				v[3] = v[7];
			}

			// fill in positions of each point
			positions.push_back(p);
		}
	}
}

void initScene(int argc, char* argv[])
{
	// background color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);

	// get program handle
	pipelineProgram = new BasicPipelineProgram;
	int ret = pipelineProgram->Init(shaderBasePath);
	if (ret != 0) abort();

	generateVertices();
	initVBO();

	std::cout << "GL error: " << glGetError() << std::endl;
}

// Note: You should combine this file
// with the solution of homework 1.

// Note for Windows/MS Visual Studio:
// You should set argv[1] to track.txt.
// To do this, on the "Solution Explorer",
// right click your project, choose "Properties",
// go to "Configuration Properties", click "Debug",
// then type your track file name for the "Command Arguments".
// You can also repeat this process for the "Release" configuration.

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

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

	// load the splines from the provided filename
	loadSplines(argv[1]);

	printf("Loaded %d spline(s).\n", numSplines);
	for (int i = 0; i < numSplines; i++)
		printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

	// do initialization
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();

	return 0;
}

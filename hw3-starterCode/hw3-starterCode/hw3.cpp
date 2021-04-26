/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: April Chang
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <glm/glm.hpp>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

#define PI 3.1415926535f
#define MIN_Z -1e20f
#define EPSILON 1e-5f

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

Ray generateRayFromCamera(int x, int y)
{
	Ray ray;
	glm::vec3 point;

	double a = (double) WIDTH / HEIGHT;
	double tanHalfFOV = tan((fov / 2) * PI / 180);

	point.x = -a * tanHalfFOV + (2 * a * tanHalfFOV * ((double) x / WIDTH));
	point.y = -tanHalfFOV + 2 * tanHalfFOV * ((double) y / HEIGHT);
	point.z = -1;

	ray.direction = glm::normalize(point);
	return ray;
}

bool intersectSphere(Ray& ray, Sphere& sphere, glm::vec3& intersection)
{
	float b, c;
	glm::vec3 spherePos(sphere.position[0], sphere.position[1], sphere.position[2]);
	glm::vec3 d = ray.origin - spherePos;
	b = 2 * (ray.direction.x * d.x + ray.direction.y * d.y + ray.direction.z * d.z);
	c = pow(d.x, 2) + pow(d.y, 2) + pow(d.z, 2) - pow(sphere.radius, 2);
	float val = pow(b, 2) - (4 * c);

	if (val < 0) return false; // no intersection

	val = sqrt(val);

	float t0, t1;
	t0 = (-b - val) / 2;
	t1 = (-b + val) / 2;

	if (t0 >= 0 && t1 >= 0)
	{
		t0 = glm::min(t0, t1);
		intersection = ray.origin + ray.direction * t0;
		return true;
	}
	return false;
}

double getTriangleArea(glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	glm::vec3 cross = glm::cross((b - a), (c - a));
	float val = glm::length(cross);
	if (val >= 0)
		return (0.5 * val);
	else
		return 0.0;
}

bool pointInTriangle(glm::vec3& intersection, Triangle& triangle)
{
	glm::vec3 c0(triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
	glm::vec3 c1(triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
	glm::vec3 c2(triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);
	
	//calculate the barycentric co efficients
	double area = getTriangleArea(c0, c1, c2);
	double alpha = getTriangleArea(intersection, c1, c2) / area;
	double beta = getTriangleArea(c0, intersection, c2) / area;
	double gamma = getTriangleArea(intersection, c0, c1) / area;

	return ((alpha + beta + gamma) >= (1.0 - EPSILON / 2.0) && (alpha + beta + gamma) <= (1.0 + EPSILON / 2.0));
}

bool intersectTriangle(Ray& ray, Triangle& triangle, glm::vec3& intersection)
{
	// compute the normal
	glm::vec3 a(triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
	glm::vec3 b(triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
	glm::vec3 c(triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);
	glm::vec3 normal = normalize(cross(b-a, c-a));

	float NdotD = dot(normal, ray.direction);
	if (abs(NdotD) < EPSILON) // no intersection
	{
		return false;
	}

	// compute d: ax + by + cz + d=0
	float d = - dot(normal, a);

	float NdotP = dot(normal, ray.origin);
	double t = (-(NdotP + d)) / NdotD;
	if (t < 0) // intersection behind origin
	{
		return false;
	}
	else
	{
		intersection = ray.origin + ray.direction * (float)t;
		return pointInTriangle(intersection, triangle);
	}
}

int getClosestSphereIntersection(Ray& ray, glm::vec3& closest)
{
	int hitIndex = -1; // returns -1 if no intersection
	closest.z = MIN_Z; // initialize z to min z value
	for (int i = 0; i < num_spheres; i++)
	{
		glm::vec3 intersection;
		if (intersectSphere(ray, spheres[i], intersection) && intersection.z > closest.z)
		{
			closest = intersection;
			hitIndex = i;
		}
	}
	return hitIndex;
}

int getClosestTriangleIntersection(Ray& ray, glm::vec3& closest)
{
	int hitIndex = -1; // returns -1 if no intersection
	closest.z = MIN_Z; // initialize z to min z value
	for (int i = 0; i < num_triangles; i++)
	{
		glm::vec3 intersection;
		if (intersectTriangle(ray, triangles[i], intersection) && intersection.z > closest.z)
		{
			closest = intersection;
			hitIndex = i;
		}
	}
	return hitIndex;
}

Ray generateShadowRay(glm::vec3& intersection, Light& light)
{
	Ray shadowRay;
	shadowRay.origin = intersection;
	shadowRay.direction.x = light.position[0] - intersection.x;
	shadowRay.direction.y = light.position[1] - intersection.y;
	shadowRay.direction.z = light.position[2] - intersection.z;
	shadowRay.direction = normalize(shadowRay.direction);
	return shadowRay;
}

bool isBlocked(Ray shadowRay, int sphereIndex, int triangleIndex, glm::vec3 intersection, Light& light)
{
	bool blocked = false;
	// check for sphere intersections
	for (int i = 0; i < num_spheres; i++)
	{
		if (i == sphereIndex) continue; // skip the sphere intersecting with cameraRay

		glm::vec3 shadowIntersection;
		if (intersectSphere(shadowRay, spheres[i], shadowIntersection))
		{
			glm::vec3 lightPos(light.position[0], light.position[1], light.position[2]);
			glm::vec3 toBlock = shadowIntersection - intersection;
			glm::vec3 toLight = lightPos - intersection;

			// shadowRay is blocked if toBlock is shorter than toLight
			if (glm::length(toBlock) < glm::length(toLight))
			{
				return true;
			}
		}
	}
	// check for triangle intersections
	for (int i = 0; i < num_triangles; i++)
	{
		if (i == triangleIndex) continue; // skip the triangle intersecting with cameraRay
			
		glm::vec3 shadowIntersection;
		if (intersectTriangle(shadowRay, triangles[i], shadowIntersection))
		{
			glm::vec3 lightPos(light.position[0], light.position[1], light.position[2]);
			glm::vec3 toBlock = shadowIntersection - intersection;
			glm::vec3 toLight = lightPos - intersection;

			// shadowRay is blocked if toBlock is shorter than toLight
			if (glm::length(toBlock) < glm::length(toLight))
			{
				return true;
			}
		}
	}
	return false;
}

glm::vec3 getPhongForSphere(Sphere& sphere, glm::vec3 intersection, Light& light)
{
	// compute normal
	glm::vec3 spherePos(sphere.position[0], sphere.position[1], sphere.position[2]);
	glm::vec3 normal = normalize(intersection - spherePos);

	// compute the light vector
	glm::vec3 lightPos(light.position[0], light.position[1], light.position[2]);
	glm::vec3 l = normalize(lightPos - intersection);

	// compute L.N
	float LdotN = glm::dot(l, normal);
	// clamp
	LdotN = glm::max(LdotN, 0.0f);
	LdotN = glm::min(LdotN, 1.0f);

	// compute the reflected ray
	glm::vec3 r = 2 * LdotN * normal - l;
	r = normalize(r);

	// compute v
	glm::vec3 v = normalize(-intersection);

	// compute R.V
	float RdotV = glm::dot(r, v);
	// clamp
	RdotV = glm::max(RdotV, 0.0f);
	RdotV = glm::min(RdotV, 1.0f);

	// calculate illumination
	glm::vec3 color;
	color.r = light.color[0] * ((sphere.color_diffuse[0] * LdotN) + (sphere.color_specular[0] * pow(RdotV, sphere.shininess)));
	color.g = light.color[1] * ((sphere.color_diffuse[1] * LdotN) + (sphere.color_specular[1] * pow(RdotV, sphere.shininess)));
	color.b = light.color[2] * ((sphere.color_diffuse[2] * LdotN) + (sphere.color_specular[2] * pow(RdotV, sphere.shininess)));
	
	return color;
}



glm::vec3 getPhongForTriangle(Triangle& triangle, glm::vec3 intersection, Light& light)
{
	glm::vec3 c0(triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
	glm::vec3 c1(triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
	glm::vec3 c2(triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);

	// calculate the barycentric co efficients
	float area = getTriangleArea(c0, c1, c2);
	float alpha = getTriangleArea(intersection, c1, c2) / area;
	float beta = getTriangleArea(c0, intersection, c2) / area;
	float gamma = 1.0f - alpha - beta;

	// interpolate n
	glm::vec3 normal;
	normal.x = (alpha * triangle.v[0].normal[0]) + (beta * triangle.v[1].normal[0]) + (gamma * triangle.v[2].normal[0]);
	normal.y = (alpha * triangle.v[0].normal[1]) + (beta * triangle.v[1].normal[1]) + (gamma * triangle.v[2].normal[1]);
	normal.z = (alpha * triangle.v[0].normal[2]) + (beta * triangle.v[1].normal[2]) + (gamma * triangle.v[2].normal[2]);
	normal = normalize(normal);

	// interpolate diffuse
	glm::vec3 diffuse;
	diffuse.x = (alpha * triangle.v[0].color_diffuse[0]) + (beta * triangle.v[1].color_diffuse[0]) + (gamma * triangle.v[2].color_diffuse[0]);
	diffuse.y = (alpha * triangle.v[0].color_diffuse[1]) + (beta * triangle.v[1].color_diffuse[1]) + (gamma * triangle.v[2].color_diffuse[1]);
	diffuse.z = (alpha * triangle.v[0].color_diffuse[2]) + (beta * triangle.v[1].color_diffuse[2]) + (gamma * triangle.v[2].color_diffuse[2]);

	// interpolate specular
	glm::vec3 specular;
	specular.x = (alpha * triangle.v[0].color_specular[0]) + (beta * triangle.v[1].color_specular[0]) + (gamma * triangle.v[2].color_specular[0]);
	specular.y = (alpha * triangle.v[0].color_specular[1]) + (beta * triangle.v[1].color_specular[1]) + (gamma * triangle.v[2].color_specular[1]);
	specular.z = (alpha * triangle.v[0].color_specular[2]) + (beta * triangle.v[1].color_specular[2]) + (gamma * triangle.v[2].color_specular[2]);

	// interpolate shininess
	float shininess = (alpha * triangle.v[0].shininess) + (beta * triangle.v[1].shininess) + (gamma * triangle.v[2].shininess);

	// compute the light vector
	glm::vec3 lightPos(light.position[0], light.position[1], light.position[2]);
	glm::vec3 l = lightPos - intersection;
	l = normalize(l);

	// compute L.N
	float LdotN = glm::dot(l, normal);
	// clamp
	LdotN = glm::max(LdotN, 0.0f);
	LdotN = glm::min(LdotN, 1.0f);

	// compute the reflected ray
	glm::vec3 r = 2 * LdotN * normal - l;
	r = normalize(r);

	// compute v
	glm::vec3 v = normalize(-intersection);

	// compute R.V
	float RdotV = glm::dot(r, v);
	// clamp
	RdotV = glm::max(RdotV, 0.0f);
	RdotV = glm::min(RdotV, 1.0f);

	// calculate illumination
	glm::vec3 lightColor(light.color[0], light.color[1], light.color[2]);
	glm::vec3 color;
	color = lightColor * ((diffuse * LdotN) + (specular * pow(RdotV, shininess)));
	return color;
}

void draw_scene()
{
  for (unsigned int x = 0; x < WIDTH; x++)
  {
    glPointSize(2.0);  
    glBegin(GL_POINTS);
    for (unsigned int y = 0; y < HEIGHT; y++)
    {
		glm::vec3 pixelColor;

		// generate ray through this pixel
		Ray cameraRay = generateRayFromCamera(x, y);
		glm::vec3 closestSphereP, closestTriangleP, closestP;
		int hitSphere = getClosestSphereIntersection(cameraRay, closestSphereP);
		int hitTriangle = getClosestTriangleIntersection(cameraRay, closestTriangleP);
		
		if (hitSphere == -1 && hitTriangle == -1) // no intersection
		{
			pixelColor = glm::vec3(1.0f, 1.0f, 1.0f);
		}
		else 
		{
			// set closest intersection point
			if (closestSphereP.z > closestTriangleP.z) // hit sphere
			{
				closestP = closestSphereP;
				hitTriangle = -1; // set hitTriangle to -1
			}
			else
			{
				closestP = closestTriangleP; // hit triangle
				hitSphere = -1; // set hitSphere to -1
			}

			// for each unblocked shadow ray, add local Phong model for that light to pixelColor
			for (Light light : lights)
			{
				Ray shadowRay = generateShadowRay(closestP, light);
				if (!isBlocked(shadowRay, hitSphere, hitTriangle, closestP, light))
				{
					if (hitSphere != -1) // hit sphere
					{
						pixelColor += getPhongForSphere(spheres[hitSphere], closestP, light);
					}
					else if (hitTriangle != -1) // hit triangle
					{
						pixelColor += getPhongForTriangle(triangles[hitTriangle], closestP, light);
					}
				}
			}
		}
		
		// add ambient light
		pixelColor.r += ambient_light[0];
		pixelColor.g += ambient_light[1];
		pixelColor.b += ambient_light[2];

		// scale rgb values
		pixelColor *= 255.0f;

		// plot pixel with clamped values
		plot_pixel(x, y, (unsigned char) glm::min(ceil(pixelColor.r), 255.0f), 
			(unsigned char)glm::min(ceil(pixelColor.g), 255.0f),
			(unsigned char)glm::min(ceil(pixelColor.b), 255.0f));
    }
    glEnd();
    glFlush();
  }
	
  printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}


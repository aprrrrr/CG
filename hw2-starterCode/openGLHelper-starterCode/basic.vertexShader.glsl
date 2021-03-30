#version 150

in vec3 position;
in vec4 color;
in vec3 positionLeft;
in vec3 positionRight;
in vec3 positionUp;
in vec3 positionDown;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform int mode;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  if (mode == 0)
  {
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
	col = color;
  }
  else
  {
	float eps = 0.00001f;
	vec3 smoothed = (positionLeft + positionRight + positionUp + positionDown) / 4;
	gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
	col = smoothed.y * max(color, vec4(eps)) / max(position.y, eps);
  }
}


#version 330 core
layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertColor;
layout (location = 2) in vec3 vertNormal;
layout (location = 3) in vec2 myTexCoord;

out vec3 fragPos;
out vec3 fragColor;
out vec2 texCoord;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {

	fragPos     = vec3(model * vec4(vertPos, 1.0));
	fragColor = vertColor;
	Normal      = mat3(transpose(inverse(model))) * vertNormal;
	texCoord = vec2(myTexCoord.x, myTexCoord.y);

	gl_Position = proj * view * vec4(fragPos, 1.0);
	//gl_Position = proj * view * model * vec4(vertPos, 1.0);
}

/* vim: set tabstop=2:softtabstop=2:shiftwidth=2:noexpandtab */


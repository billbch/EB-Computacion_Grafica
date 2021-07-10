#include <files.hpp>
#include <model.hpp>
#include <cam.hpp>

//#include "glutil_cube.h"
//#include "figure.h"
//#include "camera.h"

#include<figures.h>

#include <vector>
#include <math.h>
#include <stdlib.h>
#include <time.h>

const u32 FSIZE = sizeof(f32);
const u32 ISIZE = sizeof(i32);
const u32 SCR_WIDTH  = 1280;
const u32 SCR_HEIGHT = 720;
const f32 ASPECT     = (f32)SCR_WIDTH / (f32)SCR_HEIGHT;

Cam* cam;
int contadorx = 0;
int contadoraso = 0;
f32  deltaTime  = 0.0f;
f32  lastFrame  = 0.0f;
bool wireframe  = false;

/// <summary>
float* fNoiseSeed2D = nullptr;
float* fPerlinNoise2D = nullptr;
int nOctaveCount = 5;
float fScalingBias = 2.0f;
/// 
/// 
/// </summary>
/// <param name="window"></param>
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cam->processKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cam->processKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cam->processKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cam->processKeyboard(RIGHT, deltaTime);
	}
}
void key_callback(GLFWwindow*, int key, int, int act, int) {
	wireframe ^= key == GLFW_KEY_E && act == GLFW_PRESS;
}
void mouse_callback(GLFWwindow* window, f64 xpos, f64 ypos) {
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		cam->movePov(xpos, ypos);
	} else {
		cam->stopPov();
	}
}
void scroll_callback(GLFWwindow*, f64, f64 yoffset) {
	cam->processScroll((f32)yoffset);
}

void PerlinNoise2D(u32 nWidth, u32 nHeight, float* fSeed, int nOctaves, float fBias, float* fOutput) {

	for (int x = 0; x < nWidth; x++)
		for (int y = 0; y < nHeight; y++)
		{
			float fNoise = 0.0f;
			float fScaleAcc = 0.0f;
			float fScale = 1.0f;

			for (int o = 0; o < nOctaves; o++)
			{
				int nPitch = nWidth >> o;
				int nSampleX1 = (x / nPitch) * nPitch;
				int nSampleY1 = (y / nPitch) * nPitch;

				int nSampleX2 = (nSampleX1 + nPitch) % nWidth;
				int nSampleY2 = (nSampleY1 + nPitch) % nWidth;

				float fBlendX = (float)(x - nSampleX1) / (float)nPitch;
				float fBlendY = (float)(y - nSampleY1) / (float)nPitch;

				float fSampleT = (1.0f - fBlendX) * fSeed[nSampleY1 * nWidth + nSampleX1] + fBlendX * fSeed[nSampleY1 * nWidth + nSampleX2];
				float fSampleB = (1.0f - fBlendX) * fSeed[nSampleY2 * nWidth + nSampleX1] + fBlendX * fSeed[nSampleY2 * nWidth + nSampleX2];

				fScaleAcc += fScale;
				fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
				fScale = fScale / fBias;
			}
			// Scale to seed range
			fOutput[y * nWidth + x] = fNoise / fScaleAcc;
		}

}


i32 main() {
	GLFWwindow* window = glutilInit(3, 3, SCR_WIDTH, SCR_HEIGHT, "Cubito");
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);


	glm::vec3 camPos = glm::vec3(0.0f,0.0f,0.0f);
	cam = new Cam(camPos);

	Files* files = new Files("bin", "resources/textures", "resources/objects");

	Shader* shader = new Shader(files, "shader.vert", "shader.frag");
	Shader* shaderCubes = new Shader(files, "shaderC.vert", "shaderC.frag");
	Shader* rockShader = new Shader(files, "shaderC.vert", "shaderC.frag");


	Model*  monito = new Model(files, "monito/monito.obj");
	Model*  bat    = new Model(files, "bat/bat.obj");
	Model*  rock   = new Model(files, "rock/rock.obj");

	Cube* cubex = new Cube();

	srand(time(0));
	u32 n = 60;
	u32 capas = 5;
	
	fNoiseSeed2D = new float[n * n];
	fPerlinNoise2D = new float[n * n];
	for (int i = 0; i < n * n; i++) {
		fNoiseSeed2D[i] = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 4));//(float)rand() / (float)RAND_MAX;
	}
	PerlinNoise2D(n, n, fNoiseSeed2D, nOctaveCount, fScalingBias, fPerlinNoise2D);

	std::vector<glm::vec3> positions(n * n * capas);
	std::vector<glm::vec3> surface(n * n);
	for (u32 k = 0; k < capas; ++k) {
		for (u32 i = 0; i < n; ++i) {
			for (u32 j = 0; j < n; ++j) {
				f32 x = i - n / 2.0f;
				f32 z = j - n / 2.0f;
				f32 y = (u32)(fPerlinNoise2D[j * n + i] * 16.0f) - 1.0f * k;

				positions[(k * n * n) + i * n + j] = glm::vec3(x, y, z);
			}
		}
	}

	for (u32 i = 0; i < n * n; ++i) {
		surface[i] = positions[i];
	}

	u32 vbo, vao, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, cubex->getVSize() * FSIZE,
		cubex->getVertices(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubex->getISize() * ISIZE,
		cubex->getIndices(), GL_STATIC_DRAW);

	// posiciones
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, cubex->len(), cubex->skip(0));
	glEnableVertexAttribArray(0);
	// normales
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, cubex->len(), cubex->skip(6));
	glEnableVertexAttribArray(2);
	// textures
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, cubex->len(), cubex->skip(9));
	glEnableVertexAttribArray(3);

	u32 texture1 = shaderCubes->loadTexture("grass.jpg");
	u32 texture2 = shaderCubes->loadTexture("stone.jpg");
	//u32 texture3 = shader->loadTexture("snow.jpg", 2);

	glm::vec3 lightPos   = glm::vec3(0.0f,50.0f,0.0f);
	glm::vec3 lightColor = glm::vec3(1.0f);
	glm::vec3 batpos = glm::vec3(0.0f, 50.0f, 0.0f);


	u32 amount = 1000;
	std::vector<glm::vec3> model_pos(amount);
	glm::mat4* models = new glm::mat4[amount];
	srand(glfwGetTime());
	f32 radius = 15.0f;
	f32 offset = 2.5f;
	for (u32 i = 0; i < amount; ++i) {
		glm::mat4 model = glm::mat4(1.0f);

		f32 x = (rand() % 400)-200; 

		f32 y = (rand() % 400)-200;
		
		f32 z = (rand() % 400)-200; 

		model_pos[i] = glm::vec3(x, y, z);
		model = glm::translate(model, model_pos[i]);
		
		f32 scale = (rand() % 100) / 20 + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		f32 rotAngle = (rand() % 360);
		model = glm::rotate(model, rotAngle, { 0.4,0.6,0.8 });

		models[i] = model;
		
	}
	int auxi[100];
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		contadoraso += 1;
		f32 currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glBindTexture(GL_TEXTURE_2D, texture1);

		processInput(window);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glPolygonMode(GL_FRONT_AND_BACK, wireframe? GL_LINE: GL_FILL);
	

		shaderCubes->use();
		glm::mat4 projection = glm::perspective(cam->zoom, ASPECT, 0.1f, 100.0f);
		shaderCubes->setMat4("proj", projection);

		glm::mat4 view = glm::mat4(1.0f);
		//view = glm::lookAt(position, position + front, up);
		shaderCubes->setMat4("view", cam->getViewM4());

		//lightPos = cam->getPos();
		lightPos.x = float(n / 2) * (cos(currentFrame) + sin(currentFrame));
		lightPos.z = float(n / 2) * (cos(currentFrame) - sin(currentFrame));
		shaderCubes->setVec3("xyz", lightPos);
		shaderCubes->setVec3("xyzColor", lightColor);
		shaderCubes->setVec3("xyzView", cam->pos);

		glBindVertexArray(vao);
		u32 contador = 0;
		for (u32 i = 0; i < positions.size(); ++i) {

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, positions[i]);
			if (positions[i][1] < surface[contador][1] - 2.0f) glBindTexture(GL_TEXTURE_2D, texture2);
			else { glBindTexture(GL_TEXTURE_2D, texture1); }

			shaderCubes->setMat4("model", model);

			glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);
			contador++;
			if (contador >= n * n) contador = 0;
		}

		shader->use();
		shader->setVec3("xyz", lightPos);
		shader->setVec3("xyzColor", lightColor);
		shader->setVec3("xyzView", cam->pos);
		glm::mat4 proj = glm::perspective(cam->zoom, ASPECT, 0.1f, 100.0f);
		shader->setMat4("proj", proj);
		shader->setMat4("view", cam->getViewM4());

		//chango
		
		glm::mat4 model = glm::mat4(1.0f);
		model = translate(model, glm::vec3(0.0f, 60.0f, 0.0f));
		shader->setMat4("model", model);
		monito->Draw(shader);
		
		//bat

		glm::mat4 model1 = glm::mat4(1.0f);
		//batpos.x = (10.0f * (cos(currentFrame) - sin(currentFrame)))+cam->getposX();
		//batpos.z = (10.0f * (cos(currentFrame) + sin(currentFrame)))+cam->getposZ();
		//batpos.y = (cam->getposY() + (3.0f *(cos(currentFrame) - sin(currentFrame))));
		model1 = translate(model1, batpos);
		model1 = glm::rotate(model1, currentFrame * 1.5f, { 1.0f,0.0f,0.0f });
		shader->setMat4("model", model1);
		bat->Draw(shader);

		//rock


		//CUBOS CHOQUE
		for (u32 i = 0; i < positions.size(); ++i) {

			if (cam->getposX() < positions[i][0] + 1 && cam->getposX() > positions[i][0] - 1 
			 && cam->getposY() < positions[i][1] + 1 && cam->getposY() > positions[i][1] - 1
			 && cam->getposZ() < positions[i][2] + 1 && cam->getposZ() > positions[i][2] - 1) {

				contador += 1;

				positions[i][0] = 20;
				positions[i][1] = 20;
				positions[i][2] = 20;
				
			}
		}
		rockShader->use();
		rockShader->setVec3("xyz", lightPos);
		rockShader->setVec3("xyzColor", lightColor);
		rockShader->setVec3("xyzView", cam->pos);
		rockShader->setMat4("proj", proj);
		rockShader->setMat4("view", cam->getViewM4());
		//model = translate(model, glm::vec3(0.0f, 60.0f, 5.0f));

		//std::cout << model_pos[0][0] << " " << model_pos[0][1] << " " << model_pos[0][2]  << std::endl;
		for (u32 i = 0; i < amount; i++) {
			rockShader->setMat4("model", models[i]);
			rock->Draw(rockShader);
		}

		//MODELS CHOQUE
		for (u32 i = 0; i < model_pos.size(); ++i) {

			//model_pos[i][0] = 20;
			//model_pos[i][1] = 20;
			//model_pos[i][2] = 20;

			if (cam->getposX() < model_pos[i][0] + 3 && cam->getposX() > model_pos[i][0] - 3
				&& cam->getposY() < model_pos[i][1] + 3 && cam->getposY() > model_pos[i][1] - 3
				&& cam->getposZ() < model_pos[i][2] + 3 && cam->getposZ() > model_pos[i][2] - 3) {

				auxi[contadorx] = i;
				contadorx += 1;

				glm::mat4 model = glm::mat4(1.0f);
				
				model_pos[i] = glm::vec3(cam->getposX()-5, cam->getposY(), cam->getposZ());
				model = glm::translate(model, model_pos[i]);

				f32 scale = (rand() % 100) / 20 + 0.05;
				model = glm::scale(model, glm::vec3(scale));

				f32 rotAngle = (rand() % 360);
				model = glm::rotate(model, rotAngle, { 0.4,0.6,0.8 });

				models[i] = model;
				//std::cout << model_pos[0][0] << " " << model_pos[0][1] << " " << model_pos[0][2] <<"esteeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" <<std::endl;
			}
		}

		for (int i = 0; i < contadorx; i++) {

			glm::mat4 model = glm::mat4(1.0f);
			if (contadoraso%30==0) {
				model_pos[auxi[i]] = glm::vec3(cam->getposX() *i, cam->getposY() - 15 - 15 * i, cam->getposZ()*i);

			}
			else {
				model_pos[auxi[i]] = glm::vec3(cam->getposX(), cam->getposY() - 15 - 15 * i, cam->getposZ());
			}
			
			model = glm::translate(model, model_pos[auxi[i]]);

			//f32 scale = (rand() % 100) / 20 + 0.05;
			//model = glm::scale(model, glm::vec3(scale));

			//f32 rotAngle = (rand() % 360);
			//model = glm::rotate(model, rotAngle, { 0.4,0.6,0.8 });

			models[i] = model;
		}
		
		//std::cout << model_pos[0][0]<<" "<< model_pos[0][1]<<" "<< model_pos[0][2] << std::endl;
		//std::cout << cam->getposX() <<" "<< cam->getposY()<<" "<< cam->getposZ() << std::endl;
		std::cout << contadorx << std::endl;


		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	delete shader;
	delete cubex;

	//delete[] models;
	delete bat;
	delete cam;
	delete shader;
	delete monito;

	return 0;
}

/* vim: set tabstop=2:softtabstop=2:shiftwidth=2:noexpandtab */


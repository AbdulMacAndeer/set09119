#pragma once
// Math constants
#define _USE_MATH_DEFINES
#include <cmath>  
#include <random>

// Std. Includes
#include <string>
#include <time.h>
#include  <iostream>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include "glm/ext.hpp"

// Other Libs
#include "SOIL2/SOIL2.h"

// project includes
#include "Application.h"
#include "Shader.h"
#include "Mesh.h"

// time
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Vectors for physics
//velocity, acc, force
glm::vec3 v, a, f;
//mass
GLfloat mass = 1.0f;
//gravity constant
glm::vec3 g = glm::vec3(0.0f, -9.8f, 0.0f);
GLfloat scale = 7.0f;

// Random float
float randf(float lo, float hi)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = hi - lo;
	float r = random * diff;
	return lo + r;
}

// main function
int main()
{
	// create application
	Application app = Application::Application();
	app.initRender();
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 20.0f));
			
	// create ground plane
	Mesh plane = Mesh::Mesh(Mesh::QUAD);
	// scale it up x5
	plane.scale(glm::vec3(5.0f, 5.0f, 5.0f));
	Shader lambert = Shader("resources/shaders/physics.vert", "resources/shaders/physics.frag");
	Shader transpar = Shader("resources/shaders/physics.vert", "resources/shaders/solid_transparent.frag");
	plane.setShader(lambert);

	// create particle
	Mesh particle1 = Mesh::Mesh(Mesh::QUAD);
	//scale it down (x.1), translate it up by 2.5 and rotate it by 90 degrees around the x axis
	particle1.translate(glm::vec3(0.0f, 2.5f, 0.0f));
	particle1.scale(glm::vec3(.1f, .1f, .1f));
	particle1.rotate((GLfloat) M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
	particle1.setShader(Shader("resources/shaders/solid.vert", "resources/shaders/solid_blue.frag"));
	
	// create bound box
	Mesh cube = Mesh::Mesh("resources/models/cube.obj");
	cube.scale(glm::vec3(scale));
	cube.translate(glm::vec3(0.0f, 3.5f, 0.0f));
	cube.setShader(transpar);

	// time
	GLfloat firstFrame = (GLfloat) glfwGetTime();
	
	//set initial pos
	v = glm::vec3(randf(-5.0f,5.0f));

	glm::vec3 cubePos = glm::vec3(cube.getPos() - 0.5f*scale);
	glm::vec3 cubeSize = glm::vec3(cube.getPos() +scale);
	float energyDrain = 1.05f;
	float airDensity = 1.225f;
	float cubeCd = 1.05f;
	glm::vec3 windV = glm::vec3(50.0f, 0.0f, 0.0f);

	// Game loop
	while (!glfwWindowShouldClose(app.getWindow()))
	{
		// Set frame time
		GLfloat currentFrame = (GLfloat)  glfwGetTime() - firstFrame;
		// the animation can be sped up or slowed down by multiplying currentFrame by a factor.
		currentFrame *= 1.5f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		/*
		**	INTERACTION
		*/
		// Manage interaction
		app.doMovement(deltaTime);

		/*
		**	SIMULATION
		*/
		//Task 1
		// compute force
		f = g;
		// compute acceleration
		a = f/mass;
		//Update Position
		v += deltaTime*a;

		//Task 2 - Collisions
		//Get vectors of positions 
		glm::vec3 p1pos = glm::vec3(particle1.getPos());
		glm::vec3 cubepos = glm::vec3(cube.getPos());
		
		////if particle exceeds X
		//if ((p1pos.x >= (cubepos.x + 0.5f*scale)) || (p1pos.x <= (cubepos.x - 0.5f*scale)))
		//{
		//	v *= glm::vec3(-1.0f, 1.0f, 1.0f) / 1.05f;
		//}
		////if particle exceeds Y - cube.getTranslate()[3][1] - cube.getPos().y)
		//if ((p1pos.y <= (cubepos.y - 0.5f*scale)) || (p1pos.y >= (cubepos.y + 0.5f*scale)))
		//{
		//	v *= glm::vec3(1.0f, -1.0f, 1.0f) / 1.05f;
		//}
		////if particle exceeds z
		//if ((p1pos.z <= (cubepos.z - 0.5f*scale)) || (p1pos.z >= (cubepos.z + 0.5f*scale)))
		//{
		//	v *= glm::vec3(1.0f, 1.0f, -1.0f) / 1.05f;
		//}
		//

		particle1.translate(deltaTime*v);
		for (int i = 0; i < 3; i++)
		{
			if (particle1.getPos()[i] >= cubePos[i])
			{
				glm::vec3 particlePosition = glm::vec3(particle1.getPos().x, particle1.getPos().y, particle1.getPos().z);
				particlePosition[i] = cubePos[i];
				particle1.setPos(particlePosition);
				v[i] = -v[i] / energyDrain;
			}
			else if (particle1.getPos()[i] <= (cubePos[i] - cubeSize[i]))
			{
				glm::vec3 particlePosition = glm::vec3(particle1.getPos().x, particle1.getPos().y, particle1.getPos().z);
				particlePosition[i] = cubePos[i] - cubeSize[i];
				particle1.setPos(particlePosition);
				v[i] = -v[i] / energyDrain;
			}
		}
		
		////TASK 3
		//// Add forces
		//glm::vec3 fgravity = mass * g;
		//glm::vec3 faero = 0.5f * 1.225f * glm::vec3(50.0f, 0.0f, 0.0f) * glm::vec3(50.0f, 0.0f, 0.0f) * * (0.1*0.1) * -1.0f;
		//glm::vec3 totalForce = fgravity + faero;
		//// Compute acceleration
		//a = totalForce / mass;
		//// Integrate to calculate velocity and position
		//v = v + a * deltaTime;
		//particle1.translate(v * deltaTime);

		/*
		**	RENDER 
		*/		
		// clear buffer
		app.clear();
		// draw groud plane
		app.draw(plane);
		// draw particles
		app.draw(particle1);	

		// draw bounds objects
		app.draw(cube);

		app.display();
	}

	app.terminate();

	return EXIT_SUCCESS;
}


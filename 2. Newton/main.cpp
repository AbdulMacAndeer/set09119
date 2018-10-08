#pragma once
// Math constants
#define _USE_MATH_DEFINES
#include <cmath>  
#include <random>
#include <iostream>

// Std. Includes
#include <string>
#include <time.h>

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

//Vars
int selection;
float mass;
//velocity, acc, force, gravity(-9.8 on y-axis)
glm::vec3 v, a, f, g = glm::vec3(0.0f, -9.8f, 0.0f);
glm::vec3 boundScale = glm::vec3(5.0f);


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
	//Note to user
	std::cout << "Press E to reset";

	//Create application
	Application app = Application::Application();
	app.initRender();
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 20.0f));

	//Shaders
	Shader lambert = Shader("resources/shaders/physics.vert", "resources/shaders/physics.frag");
	Shader transparent= Shader("resources/shaders/physics.vert", "resources/shaders/solid_transparent.frag");
	
	//Create ground plane mesh
	Mesh plane = Mesh::Mesh(Mesh::QUAD);
	// scale it up x5
	plane.scale(boundScale);
	//Apply Shader
	plane.setShader(lambert);
	
	// create particle
	Mesh particle1 = Mesh::QUAD;//  Mesh::Mesh("resources/models/sphere.obj");
	//scale it down (x.1), translate it up by 2.5 and rotate it by 90 degrees around the x axis
	particle1.translate(glm::vec3(0.0f, 2.5f, 0.0f));
	particle1.scale(glm::vec3(.1f, .1f, .1f));
	particle1.rotate((GLfloat)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
	particle1.setShader(Shader("resources/shaders/solid.vert", "resources/shaders/solid_red.frag"));

	// create demo objects (a cube and a sphere)
	Mesh sphere = Mesh::Mesh("resources/models/sphere.obj");
	sphere.translate(glm::vec3(-1.0f, 1.0f, 0.0f));
	sphere.setShader(lambert);
	Mesh cube = Mesh::Mesh("resources/models/cube.obj");
	cube.translate(glm::vec3(1.0f, .5f, 0.0f));
	cube.setShader(lambert);
	
	//Create a box for collision
	Mesh bounds = Mesh::Mesh("resources/models/cube.obj");
	bounds.translate(glm::vec3(0.0f, 2.5f, 0.0f));
	bounds.scale(boundScale);
	bounds.setShader(transparent);

	//Time
	GLfloat firstFrame = (GLfloat)glfwGetTime();

	//Initial position
	v = glm::vec3(randf(2.5f, 8.0f));
	
	//Local vars for physics
	float energyLoss = 1.25f; //Energy lost on each bounce
	float airDens = 1.225f; //Density of the air
	float crossSectional = 125.0f; //CrossSectional of Cube area
	float dragCoefficient = 1.05f;//1.05-1.15 for quad shaped particle, 0.47 for sphere
	
	//Give particle1 some mass
	mass = 1.0f;

	// Game loop
	while (!glfwWindowShouldClose(app.getWindow()))
	{
		// Set frame time
		GLfloat currentFrame = (GLfloat)glfwGetTime() - firstFrame;
		// the animation can be sped up or slowed down by multiplying currentFrame by a factor.
		currentFrame *= 1.5f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		/*
		**	INTERACTION
		*/
		// Manage interaction
		app.doMovement(deltaTime);
		//Press E to start again
		if (app.keys[GLFW_KEY_E])
		{
			particle1.setPos(glm::vec3(randf(-2.49f, 2.49f), randf(2.5f, 4.99f), randf(-2.49f, 2.49f)));
			v = glm::vec3(glm::vec3(randf(2.5f, 8.0f), randf(2.5f, 8.0f), randf(2.5f, 8.0f)));			
		}
				
		/*
		**	SIMULATION
		*/
		
		//Compute Forces
		//Areodrag
		float fDrag = airDens*0.5f*a.x*a.x*dragCoefficient*crossSectional;
		f = g + fDrag;
		//Compute Acceleration
		a = f / mass;
		//Compute Velocity
		v += deltaTime * a;

		//Get the corners of the bounds cube in a vec
		glm::vec3 corner;

		for (int i = 0; i < 3; i++)
		{
			corner[i] = (bounds.getPos()[i] + 0.5f*boundScale[i]);
		}
	
		//for each x,y,z in particle and bounds
		for (int i = 0; i < 3; i++)
		{
			//if particles x,y,z is further than bounds
			if (particle1.getPos()[i] >= corner[i])
			{
				//Get the current position
				glm::vec3 p1Pos = glm::vec3(particle1.getPos().x, particle1.getPos().y, particle1.getPos().z);
				//Set axis to corners position
				p1Pos[i] = corner[i];
				//reverse velocity of axis
				v[i] = -v[i];
				//apply loss of energy
				v /= energyLoss;
				//apply the change to position vector
				particle1.setPos(p1Pos);
			}
			//if particles x,y,z is less than bounds
			else if (particle1.getPos()[i] <= (corner[i] - boundScale[i]))
			{
				//Get the current position
				glm::vec3 p1Pos = glm::vec3(particle1.getPos().x, particle1.getPos().y, particle1.getPos().z);
				//Set axis to corners position
				p1Pos[i] = corner[i] - boundScale[i];
				//reverse velocity of axis
				v[i] = -v[i];
				//apply loss of energy
				v /= energyLoss;
				//apply the change to position vector
				particle1.setPos(p1Pos);
			}
		}	

		//Apply the translation
		particle1.translate(deltaTime*v);

		/*
		**	RENDER
		*/
		// clear buffer
		app.clear();
		// draw groud plane
		app.draw(plane);
		// draw particles
		app.draw(particle1);

		// draw demo objects
		//app.draw(cube);
		//app.draw(sphere);

		//draw the bounds
		app.draw(bounds);

		//Show
		app.display();
	}

	app.terminate();

	return EXIT_SUCCESS;
}


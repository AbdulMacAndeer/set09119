#pragma once
#pragma region Heading
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
#include "Body.h"
#include "Particle.h"

// time
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Vars
int selection;
//force, gravity(-9.8 on y-axis)
glm::vec3 f, g = glm::vec3(0.0f, -9.8f, 0.0f);
glm::vec3 boundScale = glm::vec3(5.0f);
//bools
bool energyOn = false;
//Array for particles
std::vector<Particle> particles;
#pragma endregion

#pragma region Methods
// Random float
float randf(float lo, float hi)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = hi - lo;
	float r = random * diff;
	return lo + r;
}
//Simplectic Euler integration
void integrate(Particle &par, float dt)
{
	//Calc new Velocity
	par.setVel(par.getVel() + dt * par.getAcc());
	//Translate
	par.translate(par.getVel() * dt);
}
//Forward Euler integration
void integrateForward(Particle &par, float dt)
{
	//Get prev Velocity
	glm::vec3 preVel = par.getVel();
	//Calc velocity
	par.setVel(par.getVel() + dt * par.getAcc());
	par.translate(dt * preVel);
}

#pragma endregion

// main function
int main()
{
	//Note to user
	std::cout << "Press E to reset" << std::endl;	
	std::cout << "Press R to set initial Position to 0.0f (u = 0.0f)" << std::endl;
	std::cout << "Press 1 to turn off loss of energy (off by default)" << std::endl;
	std::cout << "Press 2 to turn on loss of energy" << std::endl;
	std::cout << " " << std::endl;
	
	//Create application
	Application app = Application::Application();
	app.initRender();
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 15.0f));

	//Shaders
	Shader lambert = Shader("resources/shaders/physics.vert", "resources/shaders/physics.frag");
	Shader transparent = Shader("resources/shaders/physics.vert", "resources/shaders/solid_transparent.frag");
	Shader particleShader = Shader("resources/shaders/solid.vert", "resources/shaders/solid_green.frag");

	//Create ground plane mesh
	Mesh plane = Mesh::Mesh(Mesh::QUAD);
	//Translate
	plane.setPos(glm::vec3(0.0f));
	// scale it up x5
	plane.scale(boundScale);
	//Apply Shader
	plane.setShader(lambert);

	//Create the particles
	for (int i = 0; i < 3; i++)
	{
		//Move array pointer with new particle
		particles.push_back(Particle());
		//Translate
		particles[i].setPos(glm::vec3(-1.0f + i, 2.0f, 0.0f));
		//Rotate
		particles[i].rotate((GLfloat)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
		//apply shader
		particles[i].getMesh().setShader(particleShader);
		//Positions
		particles[i].setVel(glm::vec3(2.5f, 2.5f, 0.0f));
		//Give mass
		particles[i].setMass(1.0f);
	}
	
	//Time
	GLfloat firstFrame = (GLfloat)glfwGetTime();
	double time = 0.0;
	double dt = 0.01;
	double currentTime = (double)glfwGetTime();
	double timeAccumulator = 0.0;

	//vars for areodrag 
	float energyLoss = 1; //Energy lost on each bounce
	float airDens = 1.225f; //Density of the air
	float crossSectional = 125.0f; //CrossSectional of Cube area
	float dragCoefficient = 0.47f;//1.05-1.15 for quad shaped particle, 0.47 for sphere

	// Game loop
	while (!glfwWindowShouldClose(app.getWindow()))
	{
		// Timekeeping
		double newTime = (double)glfwGetTime();
		double frameTime = newTime - currentTime;
		currentTime = newTime;

		timeAccumulator += frameTime;

		// Do fixed updates while time available
		while (timeAccumulator >= dt)
		{
			/*
			**	INTERACTION
			*/
			//Press E to start again
			if (app.keys[GLFW_KEY_E])
			{
				for (int i = 1; i < 3; i++)
				{
					//Translate
					particles[i].setPos(glm::vec3(-1.0f + i, 2.5f, 0.0f));
					particles[i].setVel(glm::vec3(glm::vec3(2.5f, 2.5f, 0.0f)));
					app.keys[GLFW_KEY_E] = false;
				}
			}
			//Press R to set initial pos to 0
			if (app.keys[GLFW_KEY_R])
			{
				for (int i = 1; i < 3; i++)
				{
					//Translate
					particles[i].setPos(glm::vec3(-1.0f + i, 2.5f, 0.0f));
					particles[i].setVel(glm::vec3(0.0f));
					app.keys[GLFW_KEY_R] = false;
				}
			}
			if (app.keys[GLFW_KEY_1])
			{
				energyOn = false;
				std::cout << "Energy loss off" << std::endl;
				app.keys[GLFW_KEY_1] = false;
			}
			if (app.keys[GLFW_KEY_2])
			{
				energyOn = true;
				std::cout << "Energy loss on" << std::endl;
				app.keys[GLFW_KEY_2] = false;
			}

			/*
			**	SIMULATION
			*/

			for (int i = 1; i < 3; i++)
			{
				//Compute Forces
				//Areodrag
				float fDrag;
				fDrag = airDens * 0.5f* particles[i].getAcc().x * particles[i].getAcc().x *dragCoefficient*crossSectional;
				//Add forces
				f = g + fDrag;

				//if energyloss is on
				if (energyOn)
					energyLoss = 0.70f;
				else
					energyLoss = 1.0f;
				
				//Set Acceleration
				particles[i].setAcc(f / particles[i].getMass());

				//Intergrate
				if (i == 1)
				{
					integrate(particles[i], dt);
				}
				else
				{
					integrateForward(particles[i], dt);
				}

				//if particles y is further than bounds y
				if (particles[i].getPos().y <= plane.getPos().y)
				{
					//Get the current position of particle and the y of the plane
					glm::vec3 prevPos = glm::vec3(particles[i].getPos().x, plane.getPos().y, particles[i].getPos().z);
					//get velocity
					glm::vec3 prevVel = particles[i].getVel();
					//reverse velocity of axis
					prevVel.y = -particles[i].getVel().y;// *energyLoss;
					prevVel *= energyLoss;
					
					//apply the change to position vector
					particles[i].setPos(prevPos);
					particles[i].setVel(prevVel);
				}		
			}
			
			//update time step
			timeAccumulator -= dt;
			time += dt;
		}

		// Manage interaction
		app.doMovement(timeAccumulator);

		/*
		**	RENDER
		*/
		// clear buffer
		app.clear();

		// draw groud plane
		app.draw(plane);

		// draw particles
		for (Particle p : particles)
		{
			app.draw(p.getMesh());
		}
		
		//Show
		app.display();

		//Debug - Console information
		//std::cout << 1.0f / frameTime << " fps" << std::endl;
	}

	app.terminate();

	return EXIT_SUCCESS;
}


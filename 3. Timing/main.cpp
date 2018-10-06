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
#include "Body.h"
#include "Particle.h"

// time
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Vars
int selection;
float mass;
//velocity, acc, force, gravity(-9.8 on y-axis)
glm::vec3 f, g = glm::vec3(0.0f, -9.8f, 0.0f);
glm::vec3 boundScale = glm::vec3(5.0f);


// Random float
float randf(float lo, float hi)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = hi - lo;
	float r = random * diff;
	return lo + r;
}

//Semi-implicit
void integrate(Particle &p, glm::vec3 force, float dt)
{
	p.setAcc(g + force / p.getMass());
	p.setVel(p.getVel() + p.getAcc() * dt);
	if (p.getVel().length() < 0.05f) 
		p.setVel(glm::vec3(0.0f, 0.0f, 0.0f));
	p.translate(p.getVel() * dt);
}

//Euler integration
void integrateForward(Particle &p, glm::vec3 force, float dt)
{
	glm::vec3 vel;
	vel = p.getVel() + p.getAcc() * dt;
	p.setAcc(force / p.getMass());
	if (vel.length() < 0.05f)
		vel = glm::vec3(0.0f, 0.0f, 0.0f);
	p.setPos(p.getPos() + p.getVel() * dt);
	p.setVel(vel);
}

// main function
int main()
{
	std::cout << "Select 1 for Semi-implicit intergration" << std::endl;
	std::cout << "Select 2 for Euler intergration" << std::endl;
	std::cout << " " << std::endl;
	std::cin >> selection;
	
	//Create application
	Application app = Application::Application();
	app.initRender();
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 20.0f));

	//Shaders
	Shader lambert = Shader("resources/shaders/physics.vert", "resources/shaders/physics.frag");
	Shader transparent= Shader("resources/shaders/physics.vert", "resources/shaders/solid_transparent.frag");
	Shader particleShader = Shader("resources/shaders/solid.vert", "resources/shaders/solid_red.frag");

	//Create ground plane mesh
	Mesh plane = Mesh::Mesh(Mesh::QUAD);
	// scale it up x5
	plane.scale(boundScale);
	//Apply Shader
	plane.setShader(lambert);
	
	//Create the particle
	Particle p1 = Particle::Particle();
	//Translate and rotate
	p1.translate(glm::vec3(0.0f, 2.5f, 0.0f));
	p1.rotate((GLfloat)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
	//apply shader to the mesh
	p1.getMesh().setShader(particleShader);

	//Positions
	p1.setVel(glm::vec3(randf(2.5f, 8.0f))); // (u)
	
	//Give particle1 some mass
	p1.setMass(2.0f);

	//Create a box for collision
	Mesh bounds = Mesh::Mesh("resources/models/cube.obj");
	bounds.translate(glm::vec3(0.0f, 2.5f, 0.0f));
	bounds.scale(boundScale);
	bounds.setShader(transparent);

	//Time
	GLfloat firstFrame = (GLfloat)glfwGetTime();
	double time = 0.0;
	double dt = 0.01;
	double currentTime = (double)glfwGetTime();
	double timeAccumulator = 0.0;

	//vars for areodrag 
	float energyLoss = 0.70f; //Energy lost on each bounce
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
				p1.setPos(glm::vec3(randf(-2.49f, 2.49f), randf(2.5f, 4.99f), randf(-2.49f, 2.49f)));
				p1.setVel(glm::vec3(glm::vec3(randf(2.5f, 8.0f), randf(2.5f, 8.0f), randf(2.5f, 8.0f))));
			}

			/*
			**	SIMULATION
			*/

			//Compute Forces
			//Areodrag
			float fDrag = airDens * 0.5f* p1.getAcc().x * p1.getAcc().x *dragCoefficient*crossSectional;
			f = g + fDrag;

			//Compute Acceleration
			p1.setAcc(f / p1.getMass());
			
			switch (selection)
			{
			case 1:
				//intergrate
				//p1.setVel(p1.getVel() + dt * p1.getAcc());
				break;
				integrate(p1, f, dt);
			case 2:
				integrateForward(p1, f, dt);
				break;
			}

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
				if (p1.getPos()[i] >= corner[i])
				{
					//Get the current position
					glm::vec3 p1Pos = p1.getPos();
					//Set axis in contact to corners position
					p1Pos[i] = corner[i];
					//get velocity
					glm::vec3 vel = p1.getVel();
					//reverse velocity of axis
					vel[i] = -p1.getVel()[i];
					vel *= energyLoss;
					//apply the change to position vector
					p1.setPos(p1Pos);
					p1.setVel(vel);
				}
				//if particles x,y,z is less than bounds - boundScale[i];
				else if (p1.getPos()[i] <= (corner[i] - boundScale[i]))
				{
					//Get the current position
					glm::vec3 p1Pos = p1.getPos();
					//Set axis in contact to corners position
					p1Pos[i] = corner[i] - boundScale[i];
					//get velocity
					glm::vec3 vel = p1.getVel();
					//reverse velocity of axis
					vel[i] = -p1.getVel()[i]; 
					vel *= energyLoss;
					//apply the change to position vector
					p1.setPos(p1Pos);
					p1.setVel(vel);
				}
			}
			
			//Apply the translation
			p1.translate(dt * p1.getVel());

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
		// app.draw(plane);

		// draw particles
		app.draw(p1.getMesh());

		//draw the bounds
		app.draw(bounds);

		//Show
		app.display();
		
		//Debug - Console information
		std::cout << 1.0f / frameTime << " fps" << std::endl;
	}

	app.terminate();

	return EXIT_SUCCESS;
}


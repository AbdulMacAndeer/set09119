#pragma once
#pragma region Heading
// Math constants
#define _USE_MATH_DEFINES
#include <cmath>  
#include <random>
#include <iostream>
#include <numeric>

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
#include "Force.h"

// include 
using namespace std;

//Time
GLfloat deltaTime = 0.01f;
GLfloat lastFrame = 0.0f;

//Vars
int selection;
float amount;
//Scalar for room size
glm::vec3 boundScale = glm::vec3(5.0f);

//bool for task switching
bool planeOn = false;
#pragma endregion

#pragma region Methods
//Bool to string
inline const char * const BoolToString(bool b)
{
	return b ? "true" : "false";
}
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
	par.setVel(par.getVel() + par.getAcc() * dt);
	//Translate
	par.translate(par.getVel() * dt);
}
//Forward Euler integration
void integrateForward(Particle &par, float dt)
{
	//Energy loss
	float eLoss = 0.5f;
	//Get prev Velocity
	glm::vec3 preVel = par.getVel() * eLoss;
	//Calc velocity
	par.setVel(par.getVel() + dt * par.getAcc());
	par.translate(dt * preVel);
}

//Find location in circle
bool isInsideCircle(int rad, Particle &par)
{
	// Compare radius of circle with distance of its center from given point 
	if ((par.getPos().x - 0.0f) * (par.getPos().x - 0.0f) + (par.getPos().y - 0.0f) * (par.getPos().y - 0.0f) <= rad * rad)
		return true;
	else
		return false;
}
//Returns true if the Particle is in the object
bool isInsideCone(glm::vec3 coneTipPosition, glm::vec3 coneCenterLine, Particle &par, float FOVRadians, float height)
{
	//Get the difference vector
	glm::vec3 difference = par.getPos() - coneTipPosition;
	//dot product for projection
	double result = glm::dot(coneCenterLine, difference);
	//if in the view/angle from the origin of the cone
	if (result > cos(FOVRadians))
		return true;
	else
		return false;
}
//handles detecting collisions with bounds and reversing velocity of particle 
void roomCollision(Mesh &bounds, Particle &par)
{
	//Get the corners of the bounds cube in a vec
	glm::vec3 corner;
	for (int axis = 0; axis < 3; axis++)
	{
		corner[axis] = (bounds.getPos()[axis] + 0.5f*boundScale[axis]);
	}
	//for each x,y,z in particle and bounds
	for (int j = 0; j < 3; j++)
	{
		//if particles x,y,z is further than bounds
		if (par.getPos()[j] >= corner[j])
		{
			//Get the current position
			glm::vec3 prevPos = par.getPos();
			//Set axis in contact to corners position
			prevPos[j] = corner[j] - 0.05f;
			//get velocity
			glm::vec3 prevVel = par.getVel();
			//reverse velocity of axis
			prevVel[j] = -par.getVel()[j];
			//apply the change to position vector
			par.setPos(prevPos);
			par.setVel(prevVel);
		}
		//if particles x,y,z is less than bounds - boundScale[j];
		else if (par.getPos()[j] <= (corner[j] - boundScale[j]))
		{
			//Get the current position
			glm::vec3 prevPos = par.getPos();
			//Set axis in contact to corners position
			prevPos[j] = corner[j] - boundScale[j] + 0.05f;
			//get velocity
			glm::vec3 prevVel = par.getVel();
			//reverse velocity of axis
			prevVel[j] = -par.getVel()[j];
			//apply the change to position vector
			par.setPos(prevPos);
			par.setVel(prevVel);
		}
	}
}
#pragma endregion

// main function
int main()
{
	//Create application
	Application app = Application::Application();
	app.initRender();
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 20.0f));

	//Time Stuff
	GLfloat firstFrame = (GLfloat)glfwGetTime();
	double time = 0.0f;
	double dt = 0.01f;
	double currentTime = (double)glfwGetTime();
	double timeAccumulator = 0.0f;

	//Spring forces
	float stiff = 15.0f;
	float damper = 10.0;
	float rest = 0.5f;

	//Gravity
	Gravity* g = new Gravity(glm::vec3(0.0f, -9.8f, 0.0f));
	//Wind
	glm::vec3 wind = glm::vec3(0.0f, 0.75f, 0.75f);
	//DragCoefficient
	float dragCoeff = 0.47f;
	//Density of object
	float density = 0.75f;

	//Shaders
	Shader lambert = Shader("resources/shaders/physics.vert", "resources/shaders/physics.frag");
	Shader transparent = Shader("resources/shaders/physics.vert", "resources/shaders/solid_transparent.frag");
	Shader greenShader = Shader("resources/shaders/solid.vert", "resources/shaders/solid_green.frag");
	Shader redShader = Shader("resources/shaders/solid.vert", "resources/shaders/solid_red.frag");
	Shader blueShader = Shader("resources/shaders/solid.vert", "resources/shaders/solid_blue.frag");

	//Create ground plane mesh
	Mesh plane = Mesh::Mesh(Mesh::QUAD);
	//Scale it up x5
	plane.scale(boundScale);
	//Apply Shader
	plane.setShader(lambert);

	//Create a room for collision
	Mesh bounds = Mesh::Mesh("resources/models/cube.obj");
	bounds.translate(glm::vec3(0.0f, 2.5f, 0.0f));
	bounds.scale(boundScale);
	bounds.setShader(transparent);

	//Particles
	//Create 2D arrays of particles
	Particle particles[10][10];
	//For all rows
	for (int i = 0; i < 10; i++)
	{
		//for all columns
		for (int j = 0; j < 10; j++)
		{
			Particle particle = Particle::Particle();						//create Particle
			particle.setPos(glm::vec3(-5.0f + i, 10.0f - (0.5f*j), 0.0f));	//Set its position
			particle.rotate((GLfloat)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));	//Rotate to face cam
			particle.getMesh().setShader(blueShader);						//apply shader
			particles[i][j] = particle;										//add to array
		}
	}

	//Set Particle amount
	amount = 10;

	//Add Hookes and Forces
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			particles[i][j].addForce(g);

			if ((i != 0 && j != 0) || (i != 9 && j != 0))
			{
				if (j > 0)
				{
					//if at row 1, connect to row 0
					if (i > 0)
					{
						particles[i][j].addForce(new Hooke(&particles[i][j], &particles[i - 1][j], stiff, damper, rest));
					}
					//if its not the last row, connet to the next row
					if (i < 9)
					{
						particles[i][j].addForce(new Hooke(&particles[i][j], &particles[i + 1][j], stiff, damper, rest));

						//wind simulation - pointer errors
						Drag* fwind = new Drag(&particles[i][j], &particles[i + 1][j], &particles[i + 1][j - 1], &wind, dragCoeff, density);

						//apply forces						
						//particles[i][j].addForce(fwind);
						//particles[i + 1][j].addForce(fwind);
						//particles[i + 1][j - 1].addForce(fwind);

					}
					//connect to col on right
					if (j < 9)
					{
						particles[i][j].addForce(new Hooke(&particles[i][j], &particles[i][j + 1], stiff, damper, rest));
					}
					//connect to col on left
					particles[i][j].addForce(new Hooke(&particles[i][j], &particles[i][j - 1], stiff, damper, rest));
				}
			}
		}
	}

#pragma region GameLoop
	// Game loop
	while (!glfwWindowShouldClose(app.getWindow()))
	{
		// Timekeeping
		GLfloat newTime = (double)glfwGetTime();
		GLfloat frameTime = newTime - currentTime;
		currentTime = newTime;
		//frameTime *= 0.25f;
		timeAccumulator += frameTime;

		// Do fixed updates while time available
		while (timeAccumulator >= dt)
		{
			/*
			**	INTERACTION
			*/

			// Manage interaction
			app.doMovement(dt);

			/*
			**	SIMULATION
			*/
			//For 2d array
			for (int i = 0; i < amount; i++)
			{
				for (int j = 1; j < amount; j++)
				{
					// Calculate acceleration
					particles[i][j].setAcc(particles[i][j].applyForces(particles[i][j].getPos(), particles[i][j].getVel(), time, dt));

					// Integrate to calculate new velocity and position
					integrate(particles[i][j], dt);

					// Plane collision
					if (particles[i][j].getPos().y <= plane.getPos().y)
					{
						particles[i][j].setPos(1, plane.getPos().y);
					}
				}
			}

			//update time step
			timeAccumulator -= dt;
			time += dt;
		}

		/*
		**	RENDER
		*/
		// clear buffer
		app.clear();

		// draw groud plane
		app.draw(plane);

		// draw the 2d array of particles
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				app.draw(particles[i][j].getMesh());
			}
		}

		//Show
		app.display();
	}
#pragma endregion

	app.terminate();

	return EXIT_SUCCESS;
}

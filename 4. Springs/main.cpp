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
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Vars
int selection;
float amount;
//Forces, gravity(-9.8 on y-axis)
glm::vec3 f, fGravity = glm::vec3(0.0f, -9.8f, 0.0f);
//Scalar for room size
glm::vec3 boundScale = glm::vec3(5.0f);
//Bools
bool gravityOn = false;
//Vector that store particles
vector<Particle> particles;
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
	par.setVel(par.getVel() + par.getAcc() * dt);
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
bool isInsideCone(glm::vec3 coneTipPosition, glm::vec3 coneCenterLine,  Particle &par, float FOVRadians, float height)
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
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 15.0f));

	//Time Stuff
	GLfloat firstFrame = (GLfloat)glfwGetTime();
	double time = 0.0;
	double dt = 0.01;
	double currentTime = (double)glfwGetTime();
	double timeAccumulator = 0.0;

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

	//Top Particle
	Particle topParticle = Particle::Particle();//create object
	topParticle.setPos(glm::vec3(0.0f, 10.0f, 0.0f));//Set position
	topParticle.rotate((GLfloat)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));//Rotate
	topParticle.getMesh().setShader(blueShader);//Set Shader

	//Set Particle amount
	amount = 5;
	particles.push_back(topParticle);

	//Create the particles
	for (int i = 1; i < amount; i++)
	{
		//Move array pointer with new particle
		particles.push_back(Particle());
		//Translate
		particles[i].setPos(glm::vec3(0.0f, (particles[i - 1].getPos().y - 1.0f), 0.0f));
		//Rotate
		particles[i].rotate((GLfloat)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
		//apply shader
		particles[i].getMesh().setShader(blueShader);
		//Initial Position/Velocity
		particles[i].setVel(glm::vec3(0.0f));
		//Give mass
		particles[i].setMass(1.0f);
	}
	//new loop for hooke as pointer used (particle must be created and end first loop)
	for (int i = 1; i < amount; i++)
	{
		//Create the gravit force
		Gravity* g = new Gravity(glm::vec3(0.0f, -9.8f, 0.0f));
		//Apply force(no pointer)
		particles[i].addForce(g);
		//Hookes Law 
		Hooke* fsd = new Hooke(&particles[i], &particles[i-1], 20.0f *i*i, 0.1f, 1.0f);
		//Add hook to particle(no pointer)
		particles[i].addForce(fsd);
	}
	
	#pragma region GameLoop
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
			// Manage interaction
			app.doMovement(timeAccumulator);

			/*
			**	SIMULATION
			*/
			for (int i = 0; i < amount; i++)
			{
				//Calculate Acceleration
				particles[i].setAcc(particles[i].applyForces(particles[i].getPos(), particles[i].getVel(), time, dt));								
				//Intergrate
				integrate(particles[i], dt*0.1);				
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

		// draw particles
		app.draw(topParticle.getMesh());//Top Particle

		for (Particle p : particles) //hanging particles
		{
			app.draw(p.getMesh());
		}

		//draw the bounds
		//app.draw(bounds);

		//Show
		app.display();

		//Debug - Console information
		//cout << 1.0f / frameTime << " fps" << endl;
	}
	#pragma endregion

	app.terminate();

	return EXIT_SUCCESS;
}
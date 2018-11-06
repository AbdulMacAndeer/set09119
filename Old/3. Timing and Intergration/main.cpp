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

// include 
using namespace std;

//Time
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Vars
int selection;
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
//Areodynamic Drag
float getDrag(float airDens, Particle &par, float dragCoEffic, float area)
{
	float a = par.getPos().x + par.getPos().y + par.getPos().z;
	a /= 3;
	float Fd = airDens * 0.5f *(a*a)*dragCoEffic*area;
	return Fd;
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
#pragma endregion

// main function
int main()
{
	//Note to user
	cout << "Press E to reset" << endl;
	cout << "Gravity is off. Press 1 to turn on" << endl;
	cout << " " << endl;

	//Create application
	Application app = Application::Application();
	app.initRender();
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 15.0f));

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
	
	//Create the particles
	for (int i = 0; i < 4; i++)
	{
		//Move array pointer with new particle
		particles.push_back(Particle());
		//Translate
		particles[i].setPos(glm::vec3(-1.5f + i, 2.5f, 0.0f));
		//Rotate
		particles[i].rotate((GLfloat)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
		//apply shader
		particles[i].getMesh().setShader(blueShader);
		//Positions
		particles[i].setVel(glm::vec3(0.0f));
		//Give mass
		particles[i].setMass(1.0f);
	}

	//Create a room for collision
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
	float airDens = 1.05f; //Density of the air
	float crossSectional = 0.125f; //CrossSectional of Cube area
	float dragCoefficient = 0.47f; //1.05-1.15 for quad shaped particle, 0.47 for sphere

	//vars for coneblow dryer
	//floats
	float coneFOV = M_PI / 5; //Field of view (36 degrees)
	float radiusConeBase = 1.5f;
	float radiusConeTop = 2.0f;
	float coneHeight = 1.5f;
	float circumConeBase = 2 * M_PI * radiusConeBase;
	//Vectors
	glm::vec3 coneTipPos = glm::vec3(0.0f, -0.2f, 0.0f);
	glm::vec3 coneDir = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 fWind = glm::vec3(0.0f);

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
				for (int i = 0; i < 4; i++)
				{
					//Translate
					particles[i].setPos(glm::vec3(-1.5f + i, 2.5f, 0.0f));
					particles[i].setVel(glm::vec3(0.0f));
					app.keys[GLFW_KEY_E] = false;
				}
			}
			if (app.keys[GLFW_KEY_1])
			{
				if (gravityOn)
				{
					gravityOn = false;
					cout << "Gravity off" << endl;
				}
				else
				{
					gravityOn = true;
					cout << "Gravity on" << endl;
				}
				app.keys[GLFW_KEY_1] = false;
			}
			/*
			**	SIMULATION
			*/

			for (int i = 0; i < 4; i++)
			{
				//Compute Forces
				//Areodrag
				glm::vec3 fDrag = glm::vec3(getDrag(airDens, particles[i], dragCoefficient, crossSectional));
				
				//Blowdryer
				//inside the cone
				if (isInsideCone(coneTipPos, coneDir, particles[i], coneFOV, coneHeight))
				{
					//Get the displacment transform for direction to send particle
					glm::vec3 displacement = particles[i].getPos() - coneTipPos;
					//Power of the force
					//Get the projection details
					float y = glm::dot(particles[i].getPos() - coneTipPos, coneDir);//From cone origin centre to particle pos
					glm::vec3 projection;
					projection.y = y; //From cone centre to particle pos
					projection.x = (y / coneHeight) * radiusConeBase; //Radius size of particle pos
					projection.z = 0.0f;
					float xzFade = 1 - (glm::length((particles[i].getPos() - coneTipPos) - (projection.x * coneDir)));
					float windFade = (1 - projection.y / coneHeight) * (xzFade / projection.x);
					//set the force with a normalized vector of the displacement and apply wind power fade off
					fWind = glm::normalize(displacement) * windFade;
					//scale the force for effect
					fWind *= 30;									
				}				
				else
				{
					//stop the force
					fWind *= 0.25f;
				}

				//Add forces
				f = fDrag + fWind;
				
				//add gravity if user toggles
				if (gravityOn)
					f += fGravity;

				//Set Acceleration
				particles[i].setAcc(f / particles[i].getMass());

				//Intergrate
				integrate(particles[i], dt);
				
				#pragma region Collision with Bounds of Room	
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
					if (particles[i].getPos()[j] >= corner[j])
					{
						//Get the current position
						glm::vec3 prevPos = particles[i].getPos();
						//Set axis in contact to corners position
						prevPos[j] = corner[j] - 0.05f;
						//get velocity
						glm::vec3 prevVel = particles[i].getVel();
						//reverse velocity of axis
						prevVel[j] = -particles[i].getVel()[j];
						//apply the change to position vector
						particles[i].setPos(prevPos);
						particles[i].setVel(prevVel);
					}
					//if particles x,y,z is less than bounds - boundScale[j];
					else if (particles[i].getPos()[j] <= (corner[j] - boundScale[j]))
					{
						//Get the current position
						glm::vec3 prevPos = particles[i].getPos();
						//Set axis in contact to corners position
						prevPos[j] = corner[j] - boundScale[j] + 0.05f;
						//get velocity
						glm::vec3 prevVel = particles[i].getVel();
						//reverse velocity of axis
						prevVel[j] = -particles[i].getVel()[j];
						//apply the change to position vector
						particles[i].setPos(prevPos);
						particles[i].setVel(prevVel);
					}
				}
				#pragma endregion
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

		app.draw(particles[1].getMesh());
		app.draw(particles[2].getMesh());
		//draw the bounds
		app.draw(bounds);

		//Show
		app.display();

		//Debug - Console information
		//cout << 1.0f / frameTime << " fps" << endl;
	}

	app.terminate();

	return EXIT_SUCCESS;
}
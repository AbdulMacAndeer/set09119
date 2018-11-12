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
#include "RigidBody.h"

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

bool impulsed = false;

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
// Apply impulse 
void applyImpulse(RigidBody &rb, glm::vec3 magnitude, glm::vec3 r, glm::vec3 normal)
{
	rb.setAngVel(rb.getAngVel() + magnitude * rb.getInvInertia() * glm::cross(r, normal));
	rb.setVel(rb.getVel() + magnitude / rb.getMass() * normal);
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
	double physicsTimer = 0.0f;
	double time = 0.0f;
	double dt = 0.01f;
	GLfloat currentTime = (double)glfwGetTime();
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
		
	// impulse elements
	float e = 1.0f;
	std::vector<glm::vec3> collisionEdges; //Collision vertices
	glm::vec3 coM; //Center of mass

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

	//Create a RigidBody
	//Setup mesh
	RigidBody rb = RigidBody(); //init
	Mesh m = Mesh::Mesh(Mesh::CUBE);//create a mesh
	rb.setMesh(m);//give mesh to the rigidbody
	rb.getMesh().setShader(lambert); //set shader
	rb.setMass(1.0f); //Give it mass
	rb.scale(glm::vec3(1.0f, 3.0f, 1.0f)); //Scale it
									 
	//translate
	rb.translate(glm::vec3(0.0f, 5.0f, 0.0f));
	//set initial velocity
	rb.setVel(glm::vec3(0.0f, 0.0f, 0.0f));
	//give torque
	rb.setAngVel(glm::vec3(0.0f, 0.0f, 0.0f));
	//add gravity to Rigidbody
	//rb.addForce(g);

#pragma region GameLoop
	// Game loop
	while (!glfwWindowShouldClose(app.getWindow()))
	{
		// Timekeeping
		GLfloat newTime = (double)glfwGetTime();
		GLfloat frameTime = newTime - currentTime;
		physicsTimer += frameTime;
		currentTime = newTime;
		timeAccumulator += frameTime;

		// Do fixed updates while time available
		while (timeAccumulator >= dt)
		{
			/*
			**	INTERACTION
			*/
			if (app.keys[GLFW_KEY_SPACE])
			{

			}
			// Manage interaction
			app.doMovement(dt);

			/*
			**	SIMULATION
			//*/
			// intergration ( movement) 
			// integration (translation)
			rb.setAcc(rb.applyForces(rb.getPos(), rb.getVel(), time, dt));
			rb.setVel(rb.getVel() + dt * rb.getAcc());
			rb.translate(rb.getVel() * dt);

			//Apply forces

			// integration ( rotation )
			rb.setAngVel(rb.getAngVel() + deltaTime * rb.getAngAcc());

			// create skew symmetric matrix for w
			glm::mat3 angVelSkew = glm::matrixCross3(rb.getAngVel());

			// create 3x3 rotation matrix from rb rotation matrix
			glm::mat3 R = glm::mat3(rb.getRotate());

			// update rotation matrix
			R += deltaTime * angVelSkew *R;
			R = glm::orthonormalize(R);
			rb.setRotate(glm::mat4(R));

			glm::vec3 j = glm::vec3(-5.0f, 3.0f, 0.0f);
			if (currentTime > 2.0f && !impulsed)
			{
				rb.setVel(rb.getVel() + j / rb.getMass());
				rb.setAngVel(rb.getInvInertia() * glm::cross(rb.getPos() + glm::vec3(1.0f, 0.0f, 0.0f) - rb.getPos(), j));
				impulsed = true;
			}			
			//Collisions
			// Plane collision
			for (auto vertex : rb.getMesh().getVertices())
			{
				//mesh * coords = 3d co-orods
				glm::vec3 coordinates = rb.getMesh().getModel() * glm::vec4(vertex.getCoord(), 1.0f);

				//if collision
				if (coordinates.y <= plane.getPos().y)
				{
					//stop simulation
					//dt = 0;
					collisionEdges.push_back(coordinates);
				}
			}

			//if (!collisionEdges.empty())
			//{
			//	//coM = collisionEdges.all;
			//	//glm::vec3(rb.getPos().x, rb.getPos().y - 3.0f, rb.getPos().z);
			//	
			//	rb.translate(glm::vec3(0.0f, plane.getPos().y + 0.01f, 0.0f)); //Reset to plane position
			//	glm::vec3 r = coM - rb.getPos();//Get CoM - pos
			//	glm::vec3 n = glm::vec3(0.0f, 1.0f, 0.0f); //normal vector
			//	n = glm::normalize(n);
			//	glm::vec3 vr = rb.getVel() + glm::cross(rb.getAngVel(), r);// Relative veloctiy
			//	
			//	glm::vec3 jrC = (-(1.0f + e) * vr * n);
			//	glm::vec3 jrD = ((1 / rb.getMass()) + n * (rb.getInvInertia()*glm::cross(glm::cross(r, n), r)));
			//	glm::vec3 jr = jrC / jrD;

			//	applyImpulse(rb, jr, r, n);
			//	
			//}
			
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

		app.draw(rb.getMesh());
		
		//Show
		app.display();
	}
#pragma endregion

	app.terminate();

	return EXIT_SUCCESS;
}

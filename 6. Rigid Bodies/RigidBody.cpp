# include "RigidBody.h"


RigidBody::RigidBody()
{

	// set dynamic values
	setAcc(glm::vec3(0.0f, 0.0f, 0.0f));
	setVel(glm::vec3(0.0f, 0.0f, 0.0f));
	setAngVel(glm::vec3(0.0f, 0.0f, 0.0f));
	setAngAccl(glm::vec3(0.0f, 0.0f, 0.0f));

	// physical properties
	setMass(1.0f);
	setCor(1.0f);
}

RigidBody::~RigidBody()
{
}
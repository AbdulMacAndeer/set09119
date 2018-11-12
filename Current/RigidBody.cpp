# include "RigidBody.h"


RigidBody::RigidBody()
{
	setAcc(glm::vec3(0.0f, 0.0f, 0.0f));
	setVel(glm::vec3(0.0f, 0.0f, 0.0f));
	setAngVel(glm::vec3(0.0f, 0.0f, 0.0f));
	setAngAccl(glm::vec3(0.0f, 0.0f, 0.0f));

	setMass(1.0f);
	setCor(1.0f);
}

RigidBody::~RigidBody()
{
}

glm::mat3 RigidBody::calcInvInertia()
{
	glm::mat3 matrix = glm::mat3(0.0f);
	//Get width - X Scale
	float w = getScale()[0][0];
	//Get height - Y Scale
	float h = getScale()[1][1];
	//Get depth - Z Scale
	float d = getScale()[2][2];
	
	matrix[0][0] = getMass() * (h * h + d * d) / 12.0f;
	matrix[1][1] = getMass() * (w * w + d * d) / 12.0f;
	matrix[2][2] = getMass() * (w * w + h * h) / 12.0f;
	return glm::inverse(matrix);
}

// Overrides scale and change rhe tensor
void RigidBody::scale(const glm::vec3 & vect)
{
	Body::scale(vect);
	setInvInertia(calcInvInertia());
}

// Override setMass and change the tensor
void RigidBody::setMass(const float & m)
{
	Body::setMass(m);
	setInvInertia(calcInvInertia());
}

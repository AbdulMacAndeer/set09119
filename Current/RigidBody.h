#pragma once
#include "Body.h"

class RigidBody : public Body
{
public:
	RigidBody();
	~RigidBody();

	// set and get methods
	//Set
	void setAngVel(const glm::vec3 & omega) { m_angVel = omega; }
	void setAngAccl(const glm::vec3 & alpha) { m_angAcc = alpha; }
	void setInvInertia(const glm::mat3 &invInertia) { m_invInertia = invInertia; }
	void setMass(const float & m);
	//Get
	glm::vec3 getAngVel() { return m_angVel; }
	glm::vec3 getAngAcc() { return m_angAcc; }
	glm::mat3 getInvInertia() { return Body::getMesh().getRotate() * glm::mat4(m_invInertia) * glm::transpose(Body::getMesh().getRotate()); }
	//Set Scale
	void scale(const glm::vec3 & vect);
	
private:
	float m_density;		// Rigidbody density
	glm::mat3 m_invInertia; // Inverse inertia
	glm::vec3 m_angVel;		// Angular velocity
	glm::vec3 m_angAcc;		// Angular acceleration
	glm::mat3 calcInvInertia(); //calculates the tensor for inverse inertia 
};
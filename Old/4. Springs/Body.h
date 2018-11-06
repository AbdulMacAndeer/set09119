#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Force.h"

class Body
{
public:

	/*
	** GET METHODS
	 */
	 // mesh
	Mesh & getMesh() { return m_mesh; }

	Body::Body();
	Body::~Body();

	// transform matrices
	glm::mat4 getTranslate() const { return m_mesh.getTranslate(); }
	glm::mat4 getRotate() const { return m_mesh.getRotate(); }
	glm::mat4 getScale() const { return m_mesh.getScale(); }

	// dynamic variables
	glm::vec3 & getAcc() { return m_acc; }
	glm::vec3 & getVel() { return m_vel; }
	glm::vec3 & getPos() { return m_pos; }


	// physical properties
	float getMass() const { return m_mass; }
	float getCor() { return m_cor; }

	/*
	** SET METHODS
	*/
	// mesh
	void setMesh(Mesh m) { m_mesh = m; }

	// dynamic variables
	void setAcc(const glm::vec3 & vect) { m_acc = vect; }
	void setVel(const glm::vec3 & vect) { m_vel = vect; }
	void setVel(int i, float v) { m_vel[i] = v; } // set the ith coordinate of the velocity vector
	void setPos(const glm::vec3 & vect) { m_pos = vect; m_mesh.setPos(vect); }
	void setPos(int i, float p) { m_pos[i] = p; m_mesh.setPos(i, p); } // set the ith coordinate the position vector

	// physical properties
	void setCor(float cor) { m_cor = cor; }
	void setMass(float mass) { m_mass = mass; }
	
	/*
	** OTHER METHODS
	*/

	// transformation methods
	void translate(const glm::vec3 & vect);
	void rotate(float angle, const glm::vec3 & vect);
	void scale(const glm::vec3 & vect);

	glm::vec3 applyForces(glm::vec3 pos, glm::vec3 vel, float t, float dt);

	std::vector<Force*> getForces() { return m_forces; }
	void addForce(Force *f) { m_forces.push_back(f); }

private:
	Mesh m_mesh; // mesh used to represent the body

	float m_mass; // mass
	float m_cor; // coefficient of restitution
	std::vector<Force*> m_forces;

	glm::vec3 m_acc; // acceleration
	glm::vec3 m_vel; // velocity
	glm::vec3 m_pos; // position
};
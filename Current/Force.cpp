#include <iostream>
#include <cmath>
#include "Force.h"
#include "Body.h"
#include "glm/ext.hpp"

glm::vec3 Force::apply(float mass, const glm::vec3 &pos, const glm::vec3 &vel)
{
	return glm::vec3(0.0f);

}

/*
** GRAVITY
*/
glm::vec3 Gravity::apply(float mass, const glm::vec3 &pos, const glm::vec3 &vel)
{
	return glm::vec3(0.0f, -9.8f, 0.0f) * mass;
}

/*
** DRAG
*/
glm::vec3 Drag::apply(float mass, const glm::vec3 &pos, const glm::vec3 &vel)
{
	//Get surface normal (B-A)X(C-A)
	glm::vec3 ba, ca, cross, norms;
	//ba =  (getParticle2()->getPos() - getParticle1()->getPos);
	//ca = (getParticle3()->getPos() - getParticle2()->getPos);
	//cross = glm::cross(ba,ca);
	//norms = glm::normalize(cross);

	//Get average velocity of particles
	glm::vec3 triVel = (getParticle1()->getVel() + getParticle2()->getVel() + getParticle3()->getVel());
	triVel /= 3;
	//Apply wind force
	triVel -= getWind();
	//Get area of tri (||baXca||/2)
	float area = 0.5f * glm::length(cross);
	//get area on displacement
	area *= glm::dot(triVel, norms) / glm::length(triVel);
	return (0.5f * getDens() * glm::length(triVel*triVel) * getCoEff() * area * norms);
	//return (glm::vec3(0.0f,0.25f,0.25f));
}

// HOOKE
glm::vec3 Hooke::apply(float mass, const glm::vec3 &pos, const glm::vec3 &vel)
{
	// Transform distances and velocities from 3D to 1D fsd = ?ks(l0-l)?kd(v1?v2)
	//get distance between 2 points (p2-p1 pos)
	float length = glm::length(getParticle2()->getPos() - getParticle1()->getPos());
	//Compute 1d velocities e = V / |v|
	glm::vec3 e = (getParticle2()->getPos() - getParticle1()->getPos()) / 1;
	//Get v1 and v2 (dot procut of e and the velocity)
	float v1 = glm::dot(e, vel);
	float v2 = glm::dot(e, getParticle2()->getVel());
	// Compute 1D forces
	//float fsd = (m_ks * (1 - m_rest) - m_kd * (v1 - v2)) * e);
	float fsd = -getStiffnes() * (getRestLength() - length) - getDamperCoefficient() * (v1 - v2);
	// Return the force in 3D
	return fsd * e;
}
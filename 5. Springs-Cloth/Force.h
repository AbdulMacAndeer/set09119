#pragma once
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

class Body; // forward declaration to avoid circular dependencies

class Force
{
public:
	Force() {}
	~Force() {}

	virtual glm::vec3 apply(float mass, const glm::vec3 & pos, const glm::vec3 & vel);
};


/*
** GRAVITY CLASS
*/
class Gravity : public Force
{

public:
	// constructors
	Gravity() {}
	Gravity(const glm::vec3 & gravity) { m_gravity = gravity; }

	// get and set methods
	glm::vec3 getGravity() const { return m_gravity; }
	void setGravity(glm::vec3 gravity) { m_gravity = gravity; }

	// physics
	glm::vec3 apply(float mass, const glm::vec3& pos, const glm::vec3& vel);

private:
	glm::vec3 m_gravity = glm::vec3(0.0f, -9.8f, 0.0f);

};

/*
** DRAG CLASS
*/
class Drag : public Force
{
public:
	// constructors
	Drag() {}
	Drag(Body* b1, Body* b2, Body* b3, const glm::vec3& wind, float coEff, float dens)
	{
		m_b1 = b1; m_b2 = b2; m_b3 = b3; m_wind = wind; m_coEff = coEff; m_dens = dens;
	}
	// get and set methods

	//Particle Calls
	Body* getParticle1() { return m_b1; }
	void setParticle1(Body* b1) { m_b1 = b1; }
	Body* getParticle2() { return m_b2; }
	void setParticle2(Body* b2) { m_b2 = b2; }
	Body* getParticle3() { return m_b2; }
	void setParticle3(Body* b3) { m_b3 = b3; }
	
	//wind vector
	glm::vec3 getWind() { return m_wind; }
	void setWind(glm::vec3 wind) { m_wind = wind; }

	//Coefficient of drag
	float getCoEff() { return m_coEff; }
	void setCoEff(float coEff) { m_coEff = coEff; }

	//medium density
	float getDens() { return m_dens; }
	void setDens(float dens) { m_dens = dens; }
	
	// physics
	glm::vec3 apply(float mass, const glm::vec3 &pos, const glm::vec3 &vel);

private:

	Body* m_b1;		// Pointer to the first particle that forms the triangle 
	Body* m_b2;		// Pointer to the second particle that forms the triangle 
	Body* m_b3;		// Pointer to the third particle that forms the triangle 
	glm::vec3 m_wind;	// Velocity of the wind
	float m_coEff;			// Drag coefficient
	float m_dens;			// Medium density
};

// HOOKE CLASS
class Hooke : public Force
{
public:
	//constructor
	Hooke() {}
	Hooke(Body* b1, Body* b2, float ks, float kd, float rest)
	{
		m_ks = ks; m_kd = kd; m_rest = rest; m_b1 = b1; m_b2 = b2;
	}
	// get and set methods you can write these yourself as necessary
	Body* getParticle1() { return m_b1; }
	void setParticle1(Body* b1) { m_b1 = b1; }

	Body* getParticle2() { return m_b2; }
	void setParticle2(Body* b2) { m_b2 = b2; }

	float getRestLength() { return m_rest; }
	void setRestLength(float restLength) { m_rest = restLength; }

	float getStiffnes() { return m_ks; }
	void setStiffnes(float ks) { m_ks = ks; }

	float getDamperCoefficient() { return m_kd; }
	void setDamperCoefficient(float kd) { m_kd = kd; }

	// physics
	glm::vec3 apply(float mass, const glm::vec3 & pos, const glm::vec3 & vel);


private:
	float m_ks;		// spring stiffness
	float m_kd;		// damping coefficient
	float m_rest;	// spring rest length

	Body* m_b1; // pointer to the body connected to one extremity of the spring
	Body* m_b2; // pointer to the body connected to the other extremity
};
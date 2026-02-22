#pragma once

#include <iostream>
#include <vector>
#include "GameObject.h"
#include "Spring.h"
#include "Particle.h"
#include "SimulationParams.h"

const float R = 8.3145f;

class Softbody : public GameObject
{
private:
	float m_Volume = 0.0f;
	float m_PressureValue = 0.0f;
	unsigned int m_NoOfMoles = 0;
	std::vector<std::shared_ptr<Particle>> m_Particles;
	std::vector<std::shared_ptr<Spring>> m_Springs;
	std::vector<glm::vec3> m_InitialPositions;

public:
	Softbody(unsigned int selector, float size = 1.0f, unsigned int moles = 200);
	void Update(bool simulate, float rotX, float rotY, const SimulationParams& params);
	void Reset();

	void SetPressureValue(float pressureVal);
	void SetNoOfMoles(unsigned int n);

private:
	void AddParticles();
	void AddSprings();
	std::shared_ptr<Spring> MakeSpring(std::shared_ptr<Particle> endOne, std::shared_ptr<Particle> endTwo);

	void AddGravityForce(float gravityStrength);
	void AddSpringForce(float springK, float dampingK);
	float CalculateVolume();
	void CalculatePressureForce();
	glm::vec3 CalculateCrossProduct(const Triangle& face);
	float CalculatePressureValue(unsigned int moles);
	void Integrate(float stepSize);
};

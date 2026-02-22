#pragma once

#include <iostream>
#include <vector>
#include "GameObject.h"
#include "Spring.h"
#include "Particle.h"
#include "SimulationParams.h"
#include "ColliderBox.h"
#include "PhysicsEngine.h"

class Softbody : public GameObject
{
private:
	float m_Volume = 0.0f;
	float m_VolumeAABB = 0.0f;
	float m_VolumeSphere = 0.0f;
	float m_VolumeEllipsoid = 0.0f;
	float m_VolumeExact = 0.0f;
	float m_PressureValue = 0.0f;
	unsigned int m_NoOfMoles = 0;
	std::vector<std::shared_ptr<Particle>> m_Particles;
	std::vector<std::shared_ptr<Spring>> m_Springs;
	std::vector<glm::vec3> m_InitialPositions;

public:
	Softbody(unsigned int selector, float size = 1.0f, unsigned int moles = 500);
	void Update(bool simulate, const SimulationParams& params, const ColliderBox& collider);
	void Reset();

	void SetPressureValue(float pressureVal);
	void SetNoOfMoles(unsigned int n);
	void SetParticleMass(float mass);

	const glm::vec3* GetBoundingBox() const { return m_BoundingBox; }
	float GetVolume() const { return m_Volume; }
	float GetVolumeAABB() const { return m_VolumeAABB; }
	float GetVolumeSphere() const { return m_VolumeSphere; }
	float GetVolumeEllipsoid() const { return m_VolumeEllipsoid; }
	float GetVolumeExact() const { return m_VolumeExact; }
	float GetPressure() const { return m_PressureValue; }
	size_t GetParticleCount() const { return m_Particles.size(); }
	size_t GetSpringCount() const { return m_Springs.size(); }

private:
	void AddParticles();
	void AddSprings();
	std::shared_ptr<Spring> MakeSpring(std::shared_ptr<Particle> endOne, std::shared_ptr<Particle> endTwo);

	// Per-method simulation steps
	void ComputeVolumes(const SimulationParams& params);
	void AccumulateForces(const SimulationParams& params);
	void UpdateMeshFromParticles();
};

#include "Softbody.h"

Softbody::Softbody(unsigned int selector, float size, unsigned int moles)
{
	// load mesh: 0 = sphere, 1 = cube
	if (selector == 0) m_Mesh = std::make_shared<Mesh>();
	else if (selector == 1) m_Mesh = std::make_shared<Mesh>(cube::vertices, cube::triangles);

	m_Material = std::make_shared<Material>();

	m_Size = size;
	m_NoOfMoles = moles;

	CalculateBoundingBox();

	AddParticles();
	AddSprings();

	// Store initial positions for reset
	for (auto& p : m_Particles)
		m_InitialPositions.push_back(p->GetPosition());
}

void Softbody::AddParticles()
{
	for (auto& v : m_Mesh->GetVertices())
	{
		auto temp = std::make_shared<Particle>(v.Position);
		m_Particles.push_back(std::move(temp));
	}
}

void Softbody::AddSprings()
{
	const std::vector<Triangle>& triangles = m_Mesh->GetIndices();
	size_t size = triangles.size();
	for (unsigned int i = 0; i < size; ++i)
	{
		std::shared_ptr<Particle> p1 = m_Particles[triangles[i].vertex[0]];
		std::shared_ptr<Particle> p2 = m_Particles[triangles[i].vertex[1]];
		std::shared_ptr<Particle> p3 = m_Particles[triangles[i].vertex[2]];

		m_Springs.push_back(MakeSpring(p1, p2));
		m_Springs.push_back(MakeSpring(p2, p3));
		m_Springs.push_back(MakeSpring(p3, p1));
	}
}

std::shared_ptr<Spring> Softbody::MakeSpring(std::shared_ptr<Particle> endOne, std::shared_ptr<Particle> endTwo)
{
	return std::make_shared<Spring>(endOne, endTwo);
}

// Compute all 4 volume methods and select the active one based on params
void Softbody::ComputeVolumes(const SimulationParams& params)
{
	CalculateBoundingBox();
	m_VolumeAABB = PhysicsEngine::CalculateAABBVolume(m_BoundingBox[0], m_BoundingBox[1]);
	m_VolumeSphere = PhysicsEngine::CalculateBoundingSphereVolume(m_BoundingBox[0], m_BoundingBox[1]);
	m_VolumeEllipsoid = PhysicsEngine::CalculateBoundingEllipsoidVolume(m_BoundingBox[0], m_BoundingBox[1]);
	m_VolumeExact = PhysicsEngine::CalculateExactVolume(m_Mesh->GetIndices(), m_Mesh->GetVertices());

	switch (params.volumeMethod)
	{
	case VolumeMethod::AABB:              m_Volume = m_VolumeAABB;      break;
	case VolumeMethod::BoundingSphere:    m_Volume = m_VolumeSphere;    break;
	case VolumeMethod::BoundingEllipsoid: m_Volume = m_VolumeEllipsoid; break;
	case VolumeMethod::DivergenceTheorem: m_Volume = m_VolumeExact;     break;
	}
}

// Helper: accumulate all forces (gravity + spring/damping + pressure)
void Softbody::AccumulateForces(const SimulationParams& params)
{
	PhysicsEngine::ApplyGravity(m_Particles, params.gravityStrength);
	PhysicsEngine::ApplyExternalForce(m_Particles, params.externalForce);
	PhysicsEngine::ApplySpringDampingForces(m_Springs,
		params.springConstant, params.dampingConstant);

	ComputeVolumes(params);

	m_PressureValue = PhysicsEngine::CalculatePressure(m_Volume, params.moles);
	PhysicsEngine::ApplyPressureForce(m_Particles, m_Mesh->GetIndices(),
		m_Mesh->GetVertices(), m_PressureValue);
}

// Helper: sync mesh vertices from particle positions
void Softbody::UpdateMeshFromParticles()
{
	std::vector<Vertex> vertices;
	vertices.reserve(m_Particles.size());
	for (auto& p : m_Particles)
		vertices.push_back({ p->GetPosition(), glm::vec3(0.0f), glm::vec2(0.0f) });
	m_Mesh->SetVertices(vertices);
	CalculateBoundingBox();
}

// Paper Section 3.3: Full simulation algorithm
void Softbody::Update(bool simulate, const SimulationParams& params,
					   const ColliderBox& collider)
{
	GameObject::Update(simulate, params.objectPosition);
	SetParticleMass(params.particleMass);

	if (!simulate) return;

	// Local-space collider (subtract object translation)
	ColliderBox localCollider = collider;
	localCollider.min -= params.objectPosition;
	localCollider.max -= params.objectPosition;

	float dt = params.integrationStep;

	switch (params.integrationMethod)
	{
	case IntegrationMethod::ForwardEuler:
	{
		PhysicsEngine::ClearForces(m_Particles);
		AccumulateForces(params);
		PhysicsEngine::Integrate(m_Particles, dt);
		PhysicsEngine::ResolveCollisions(m_Particles, localCollider);
		break;
	}

	case IntegrationMethod::Midpoint:
	{
		// Save original state
		size_t n = m_Particles.size();
		std::vector<glm::vec3> origPos(n), origVel(n);
		for (size_t i = 0; i < n; i++)
		{
			origPos[i] = m_Particles[i]->GetPosition();
			origVel[i] = m_Particles[i]->GetVelocity();
		}

		// Compute forces at current state
		PhysicsEngine::ClearForces(m_Particles);
		AccumulateForces(params);

		// Half-step: move particles to midpoint
		float halfDt = dt * 0.5f;
		for (size_t i = 0; i < n; i++)
		{
			glm::vec3 a = m_Particles[i]->GetForceAccumulated() / m_Particles[i]->GetMass();
			glm::vec3 vHalf = origVel[i] + a * halfDt;
			glm::vec3 xHalf = origPos[i] + vHalf * halfDt;
			m_Particles[i]->SetVelocity(vHalf);
			m_Particles[i]->SetPosition(xHalf);
		}

		// Update mesh so pressure/volume uses half-step geometry
		UpdateMeshFromParticles();

		// Recompute forces at half-step state
		PhysicsEngine::ClearForces(m_Particles);
		AccumulateForces(params);

		// Full step from original state using half-step forces
		for (size_t i = 0; i < n; i++)
		{
			glm::vec3 aHalf = m_Particles[i]->GetForceAccumulated() / m_Particles[i]->GetMass();
			glm::vec3 newVel = origVel[i] + aHalf * dt;
			glm::vec3 newPos = origPos[i] + newVel * dt;
			m_Particles[i]->SetVelocity(newVel);
			m_Particles[i]->SetPosition(newPos);
		}

		PhysicsEngine::ResolveCollisions(m_Particles, localCollider);
		break;
	}

	case IntegrationMethod::ImplicitEuler:
	{
		size_t n = m_Particles.size();

		// 1) Collect explicit forces: gravity + external + pressure
		PhysicsEngine::ClearForces(m_Particles);
		PhysicsEngine::ApplyGravity(m_Particles, params.gravityStrength);
		PhysicsEngine::ApplyExternalForce(m_Particles, params.externalForce);

		ComputeVolumes(params);
		m_PressureValue = PhysicsEngine::CalculatePressure(m_Volume, params.moles);
		PhysicsEngine::ApplyPressureForce(m_Particles, m_Mesh->GetIndices(),
			m_Mesh->GetVertices(), m_PressureValue);

		std::vector<glm::vec3> explicitForces(n);
		for (size_t i = 0; i < n; i++)
			explicitForces[i] = m_Particles[i]->GetForceAccumulated();

		// 2) Collect spring/damping forces only (for implicit solve)
		PhysicsEngine::ClearForces(m_Particles);
		PhysicsEngine::ApplySpringDampingForces(m_Springs,
			params.springConstant, params.dampingConstant);

		// 3) Implicit integrate: explicit kick for gravity/pressure,
		//    implicit solve for stiff spring/damping forces
		PhysicsEngine::IntegrateImplicit(m_Particles, m_Springs,
			explicitForces, params.springConstant, params.dampingConstant, dt);
		PhysicsEngine::ResolveCollisions(m_Particles, localCollider);
		break;
	}
	}

	UpdateMeshFromParticles();
}

void Softbody::Reset()
{
	for (size_t i = 0; i < m_Particles.size(); ++i)
	{
		m_Particles[i]->SetPosition(m_InitialPositions[i]);
		m_Particles[i]->SetVelocity(glm::vec3(0.0f));
		m_Particles[i]->ClearForce();
	}

	std::vector<Vertex> vertices;
	for (auto& p : m_Particles)
		vertices.push_back({ p->GetPosition(), glm::vec3(0.0f), glm::vec2(0.0f) });
	m_Mesh->SetVertices(vertices);

	CalculateBoundingBox();
}

void Softbody::SetPressureValue(float pressureVal)
{
	m_PressureValue = pressureVal;
}

void Softbody::SetNoOfMoles(unsigned int n)
{
	m_NoOfMoles = n;
}

void Softbody::SetParticleMass(float mass)
{
	for (auto& p : m_Particles)
		p->SetMass(mass);
}

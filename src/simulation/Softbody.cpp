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

void Softbody::Update(bool simulate, float rotX, float rotY, const SimulationParams& params)
{
	GameObject::Update(simulate, rotX, rotY);

	if (simulate)
	{
		AddGravityForce(params.gravityStrength);
		AddSpringForce(params.springConstant, params.dampingConstant);
		CalculateVolume();
		CalculatePressureValue(params.moles);
		CalculatePressureForce();
		Integrate(params.integrationStep);

		// Update mesh vertices from particle positions
		std::vector<Vertex> vertices;
		for (auto& p : m_Particles)
			vertices.push_back({ p->GetPosition(), glm::vec3(0.0f), glm::vec2(0.0f) });

		m_Mesh->SetVertices(vertices);
		CalculateBoundingBox();
	}
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

void Softbody::AddGravityForce(float gravityStrength)
{
	for (auto& p : m_Particles)
	{
		float fy = p->GetMass() * gravityStrength;
		p->AddForce(glm::vec3(0.0f, fy, 0.0f));
	}
}

void Softbody::AddSpringForce(float springK, float dampingK)
{
	for (auto& spring : m_Springs)
	{
		auto particle1 = spring->GetEndOne();
		auto particle2 = spring->GetEndTwo();

		glm::vec3 p1p = particle1->GetPosition();
		glm::vec3 p2p = particle2->GetPosition();

		glm::vec3 diff = p1p - p2p;
		float r12d = glm::length(diff);

		if (r12d != 0)
		{
			glm::vec3 relVel = particle1->GetVelocity() - particle2->GetVelocity();

			float f = (r12d - spring->GetRestLength()) * springK +
				(glm::dot(relVel, diff) * dampingK) / r12d;

			glm::vec3 forceVec = (diff / r12d) * f;

			particle1->AddForce(-forceVec);
			particle2->AddForce(forceVec);

			spring->SetNormalVector(diff / r12d);
		}
	}
}

float Softbody::CalculateVolume()
{
	float dx = m_BoundingBox[1].x - m_BoundingBox[0].x;
	float dy = m_BoundingBox[1].y - m_BoundingBox[0].y;
	float dz = m_BoundingBox[1].z - m_BoundingBox[0].z;

	float volume = dx * dy * dz;
	m_Volume = volume;
	return volume;
}

void Softbody::CalculatePressureForce()
{
	for (auto& face : m_Mesh->GetIndices())
	{
		glm::vec3 crossProduct = CalculateCrossProduct(face);
		float magnitude = glm::length(crossProduct);

		if (magnitude == 0.0f) continue;

		glm::vec3 normalToSurface = crossProduct / magnitude;
		float area = 0.5f * magnitude;

		for (unsigned int i = 0; i < face.GetVertexCount(); ++i)
		{
			auto& particle = m_Particles[face.vertex[i]];
			glm::vec3 pressureForce = m_PressureValue * area * normalToSurface;
			particle->AddForce(pressureForce);
		}
	}
}

glm::vec3 Softbody::CalculateCrossProduct(const Triangle& face)
{
	const std::vector<Vertex>& vertices = m_Mesh->GetVertices();

	glm::vec3 v1 = vertices[face.vertex[0]].Position;
	glm::vec3 v2 = vertices[face.vertex[1]].Position;
	glm::vec3 v3 = vertices[face.vertex[2]].Position;

	return glm::cross((v2 - v1), (v3 - v1));
}

float Softbody::CalculatePressureValue(unsigned int moles)
{
	float pressureValue = (1.0f / m_Volume) * moles * R;
	m_PressureValue = pressureValue;
	return pressureValue;
}

void Softbody::Integrate(float stepSize)
{
	for (auto& particle : m_Particles)
	{
		glm::vec3 velocity = particle->GetVelocity();
		glm::vec3 position = particle->GetPosition();

		glm::vec3 acceleration = particle->GetForceAccumulated() / particle->GetMass();
		velocity += acceleration * stepSize;
		position += velocity * stepSize;
		particle->SetVelocity(velocity);
		particle->SetPosition(position);
	}
}

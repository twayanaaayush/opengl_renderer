#pragma once

#include <glm/glm.hpp>
#include "Particle.h"

struct ColliderBox
{
	glm::vec3 min;
	glm::vec3 max;
	bool enabled = true;
	float restitution = 0.5f;

	ColliderBox()
		: min(glm::vec3(-3.0f, -2.0f, -3.0f)),
		  max(glm::vec3(3.0f, 4.0f, 3.0f))
	{}

	ColliderBox(glm::vec3 bMin, glm::vec3 bMax)
		: min(bMin), max(bMax)
	{}

	// AABB vs Point collision (paper Eq. 8)
	// Returns true if point is OUTSIDE the box (hit a wall)
	bool CheckPointCollision(const glm::vec3& point) const
	{
		return (point.x <= min.x || point.x >= max.x ||
				point.y <= min.y || point.y >= max.y ||
				point.z <= min.z || point.z >= max.z);
	}

	// AABB vs AABB collision (paper Eq. 9)
	bool CheckAABBCollision(const glm::vec3& otherMin, const glm::vec3& otherMax) const
	{
		return (max.x >= otherMin.x && min.x <= otherMax.x) &&
			   (max.y >= otherMin.y && min.y <= otherMax.y) &&
			   (max.z >= otherMin.z && min.z <= otherMax.z);
	}

	// Collision response: velocity decomposition with selective reflection
	void ResolveCollision(std::shared_ptr<Particle>& particle) const
	{
		glm::vec3 pos = particle->GetPosition();
		glm::vec3 vel = particle->GetVelocity();
		bool collided = false;

		// Check each axis — clamp position, decompose & reflect velocity
		auto resolve = [&](float& p, float wall, const glm::vec3& n)
		{
			p = wall;
			glm::vec3 vn = glm::dot(vel, n) * n;
			glm::vec3 vt = vel - vn;
			vel = vt + (-restitution * vn);
			collided = true;
		};

		if (pos.x <= min.x)      resolve(pos.x, min.x, glm::vec3(-1, 0, 0));
		else if (pos.x >= max.x) resolve(pos.x, max.x, glm::vec3( 1, 0, 0));

		if (pos.y <= min.y)      resolve(pos.y, min.y, glm::vec3(0, -1, 0));
		else if (pos.y >= max.y) resolve(pos.y, max.y, glm::vec3(0,  1, 0));

		if (pos.z <= min.z)      resolve(pos.z, min.z, glm::vec3(0, 0, -1));
		else if (pos.z >= max.z) resolve(pos.z, max.z, glm::vec3(0, 0,  1));

		if (collided)
		{
			particle->SetPosition(pos);
			particle->SetVelocity(vel);
		}
	}
};

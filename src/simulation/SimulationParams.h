#pragma once

#include "ColliderBox.h"
#include <glm/glm.hpp>

enum class IntegrationMethod { ForwardEuler, Midpoint, ImplicitEuler };
enum class VolumeMethod { AABB, BoundingSphere, BoundingEllipsoid, DivergenceTheorem };

struct SimulationMetrics
{
	float physicsStepMs    = 0.0f;  // Time for physics update (ms)
	float avgPhysicsStepMs = 0.0f;  // Running average
	float maxParticleDist  = 0.0f;  // Max distance from center of mass
	int   simFrameCount    = 0;     // Frames since simulation started
	bool  diverged         = false; // True if any particle exceeds threshold
};

struct SimulationParams
{
	float particleMass    = 0.5f;
	float springConstant  = 200.0f;
	float dampingConstant = 2.0f;
	float gravityStrength = -9.8f;
	unsigned int moles    = 500;
	float integrationStep = 0.011f;
	IntegrationMethod integrationMethod = IntegrationMethod::ForwardEuler;
	VolumeMethod volumeMethod = VolumeMethod::DivergenceTheorem;

	glm::vec3 externalForce    = glm::vec3(0.0f);

	glm::vec3 objectPosition   = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 colliderPosition = glm::vec3(0.0f);

	ColliderBox collider;
	bool showColliderBox  = true;
	bool showBoundingBox  = false;
};

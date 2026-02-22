#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Particle.h"
#include "Spring.h"
#include "ColliderBox.h"
#include "SimulationParams.h"
#include "Geometry.h"

// Gas constant R (J/(mol*K)) — paper Eq. 4
const float GAS_CONSTANT_R = 8.3145f;

// PhysicsEngine encapsulates all force calculations from the paper:
//   Eq. 1: Gravity
//   Eq. 2: Spring force
//   Eq. 3: Damping force
//   Eq. 5-6: Internal pressure force
//   Eq. 7: Net force accumulation
//   Eq. 8-9: AABB collision detection & response
class PhysicsEngine
{
public:
	// Eq. 1: F_gi = m_i * g
	static void ApplyGravity(std::vector<std::shared_ptr<Particle>>& particles,
							 float gravityStrength);

	// Continuous external force applied uniformly to all particles
	static void ApplyExternalForce(std::vector<std::shared_ptr<Particle>>& particles,
								    const glm::vec3& force);

	// Eq. 2 & 3: Spring force + Damping force combined
	// F_si = Σ k_ij * (|x_j - x_i| - l_ij^0) * (x_j - x_i) / |x_j - x_i|
	// F_di = Σ k_ij * h * (v_i - v_j) projected onto spring direction
	static void ApplySpringDampingForces(std::vector<std::shared_ptr<Spring>>& springs,
										 float springK, float dampingK);

	// Eq. 5: P = V^{-1} * n * R * T  (T=1 assumed)
	static float CalculatePressure(float volume, unsigned int moles);

	// Eq. 6: F_pi = Σ a_ijk * n_hat * (1/V) * n * R * T
	static void ApplyPressureForce(std::vector<std::shared_ptr<Particle>>& particles,
								   const std::vector<Triangle>& faces,
								   const std::vector<Vertex>& vertices,
								   float pressure);

	// Volume computation methods (Addition 1: Section 3.2.3a)
	static float CalculateAABBVolume(const glm::vec3& bbMin, const glm::vec3& bbMax);
	static float CalculateBoundingSphereVolume(const glm::vec3& bbMin, const glm::vec3& bbMax);
	static float CalculateBoundingEllipsoidVolume(const glm::vec3& bbMin, const glm::vec3& bbMax);
	static float CalculateExactVolume(const std::vector<Triangle>& faces,
									  const std::vector<Vertex>& vertices);

	// Forward Euler integration: v += a*dt, x += v*dt
	static void Integrate(std::vector<std::shared_ptr<Particle>>& particles,
						  float stepSize);

	// Simplified implicit (backward Euler) integration
	// Implicit solve for spring/damping only; explicit forces (gravity, pressure)
	// are passed in separately and added as an explicit velocity kick.
	static void IntegrateImplicit(std::vector<std::shared_ptr<Particle>>& particles,
								   std::vector<std::shared_ptr<Spring>>& springs,
								   const std::vector<glm::vec3>& explicitForces,
								   float springK, float dampingK, float stepSize);

	// Eq. 8: Point vs AABB collision detection and response
	// Paper Section 3.3 Step 7(b)(c)
	static void ResolveCollisions(std::vector<std::shared_ptr<Particle>>& particles,
								  const ColliderBox& collider);

	// Clear all accumulated forces (start of each timestep)
	static void ClearForces(std::vector<std::shared_ptr<Particle>>& particles);

private:
	// Helper: compute cross product of triangle edges for normal/area
	static glm::vec3 TriangleCrossProduct(const glm::vec3& v1,
										  const glm::vec3& v2,
										  const glm::vec3& v3);
};

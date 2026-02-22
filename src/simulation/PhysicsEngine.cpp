#include "PhysicsEngine.h"
#include <unordered_map>
#include <cmath>

// Eq. 1: F_gi^t = m_i * g
void PhysicsEngine::ApplyGravity(std::vector<std::shared_ptr<Particle>>& particles,
								 float gravityStrength)
{
	for (auto& p : particles)
	{
		float fy = p->GetMass() * gravityStrength;
		p->AddForce(glm::vec3(0.0f, fy, 0.0f));
	}
}

// Continuous external force (e.g. wind, push) applied to every particle
void PhysicsEngine::ApplyExternalForce(std::vector<std::shared_ptr<Particle>>& particles,
										const glm::vec3& force)
{
	if (force.x == 0.0f && force.y == 0.0f && force.z == 0.0f) return;
	for (auto& p : particles)
		p->AddForce(force);
}

// Eq. 2: F_si^t = Σ k_ij * (|x_j - x_i| - l_ij^0) * (x_j - x_i) / |x_j - x_i|
// Eq. 3: F_di^t = Σ k_ij * h * (v_i - v_j) projected onto spring direction
void PhysicsEngine::ApplySpringDampingForces(std::vector<std::shared_ptr<Spring>>& springs,
											 float springK, float dampingK)
{
	for (auto& spring : springs)
	{
		auto p1 = spring->GetEndOne();
		auto p2 = spring->GetEndTwo();

		glm::vec3 diff = p1->GetPosition() - p2->GetPosition();
		float distance = glm::length(diff);

		if (distance == 0.0f) continue;

		glm::vec3 direction = diff / distance;
		glm::vec3 relVel = p1->GetVelocity() - p2->GetVelocity();

		// Spring force (Eq. 2) + Damping force (Eq. 3) projected onto spring axis
		float forceMagnitude = (distance - spring->GetRestLength()) * springK +
							   (glm::dot(relVel, direction)) * dampingK;

		glm::vec3 force = direction * forceMagnitude;

		// Equal and opposite forces on connected particles
		p1->AddForce(-force);
		p2->AddForce(force);

		spring->SetNormalVector(direction);
	}
}

// Eq. 5: P = V^{-1} * n * R * T  (temperature T=1 assumed for simplicity)
float PhysicsEngine::CalculatePressure(float volume, unsigned int moles)
{
	if (volume <= 0.0f) return 0.0f;
	return (1.0f / volume) * moles * GAS_CONSTANT_R;
}

// Eq. 6: F_pi^t = Σ a_ijk * n_hat * (1/V) * n * R * T
void PhysicsEngine::ApplyPressureForce(std::vector<std::shared_ptr<Particle>>& particles,
									   const std::vector<Triangle>& faces,
									   const std::vector<Vertex>& vertices,
									   float pressure)
{
	for (auto& face : faces)
	{
		glm::vec3 v1 = vertices[face.vertex[0]].Position;
		glm::vec3 v2 = vertices[face.vertex[1]].Position;
		glm::vec3 v3 = vertices[face.vertex[2]].Position;

		// Negate cross product to ensure outward-facing normals
		// (icosphere winding produces inward normals from cross(v2-v1, v3-v1))
		glm::vec3 crossProduct = -TriangleCrossProduct(v1, v2, v3);
		float magnitude = glm::length(crossProduct);

		if (magnitude == 0.0f) continue;

		glm::vec3 normal = crossProduct / magnitude;
		float area = 0.5f * magnitude;

		// Apply pressure force to each vertex of the face
		glm::vec3 pressureForce = pressure * area * normal;
		for (unsigned int i = 0; i < face.GetVertexCount(); ++i)
			particles[face.vertex[i]]->AddForce(pressureForce);
	}
}

float PhysicsEngine::CalculateAABBVolume(const glm::vec3& bbMin, const glm::vec3& bbMax)
{
	float dx = bbMax.x - bbMin.x;
	float dy = bbMax.y - bbMin.y;
	float dz = bbMax.z - bbMin.z;
	return dx * dy * dz;
}

// Bounding sphere volume: sphere that encloses the AABB
float PhysicsEngine::CalculateBoundingSphereVolume(const glm::vec3& bbMin, const glm::vec3& bbMax)
{
	glm::vec3 extent = bbMax - bbMin;
	float radius = glm::length(extent) * 0.5f;
	const float PI = 3.14159265358979f;
	return (4.0f / 3.0f) * PI * radius * radius * radius;
}

// Bounding ellipsoid volume: ellipsoid fitted to the AABB semi-axes
float PhysicsEngine::CalculateBoundingEllipsoidVolume(const glm::vec3& bbMin, const glm::vec3& bbMax)
{
	glm::vec3 extent = bbMax - bbMin;
	float a = extent.x * 0.5f;
	float b = extent.y * 0.5f;
	float c = extent.z * 0.5f;
	const float PI = 3.14159265358979f;
	return (4.0f / 3.0f) * PI * a * b * c;
}

// Addition 1: Exact volume via divergence theorem
// V = Σ (1/6) * a · (b × c)  for each triangular face
float PhysicsEngine::CalculateExactVolume(const std::vector<Triangle>& faces,
										  const std::vector<Vertex>& vertices)
{
	float volume = 0.0f;
	for (const auto& face : faces)
	{
		const glm::vec3& a = vertices[face.vertex[0]].Position;
		const glm::vec3& b = vertices[face.vertex[1]].Position;
		const glm::vec3& c = vertices[face.vertex[2]].Position;
		volume += glm::dot(a, glm::cross(b, c));
	}
	return std::fabs(volume) / 6.0f;
}

// Velocity Verlet integration
void PhysicsEngine::Integrate(std::vector<std::shared_ptr<Particle>>& particles,
							  float stepSize)
{
	for (auto& particle : particles)
	{
		glm::vec3 acceleration = particle->GetForceAccumulated() / particle->GetMass();
		glm::vec3 velocity = particle->GetVelocity() + acceleration * stepSize;
		glm::vec3 position = particle->GetPosition() + velocity * stepSize;

		particle->SetVelocity(velocity);
		particle->SetPosition(position);
	}
}

// Simplified implicit (backward Euler) integration
// Spring/damping forces are solved implicitly via Jacobians.
// Explicit forces (gravity, pressure) are applied as a direct velocity kick
// since they don't cause stiffness-related instability.
void PhysicsEngine::IntegrateImplicit(std::vector<std::shared_ptr<Particle>>& particles,
									   std::vector<std::shared_ptr<Spring>>& springs,
									   const std::vector<glm::vec3>& explicitForces,
									   float springK, float dampingK, float dt)
{
	size_t n = particles.size();

	// Apply explicit forces (gravity + pressure) as velocity kick
	for (size_t i = 0; i < n; i++)
	{
		glm::vec3 v = particles[i]->GetVelocity();
		v += (explicitForces[i] / particles[i]->GetMass()) * dt;
		particles[i]->SetVelocity(v);
	}

	// Map particle pointer -> index for Jacobian accumulation
	std::unordered_map<Particle*, size_t> indexMap;
	indexMap.reserve(n);
	for (size_t i = 0; i < n; i++)
		indexMap[particles[i].get()] = i;

	// Per-particle Jacobian accumulators
	std::vector<glm::mat3> dFdx(n, glm::mat3(0.0f));
	std::vector<glm::mat3> dFdv(n, glm::mat3(0.0f));

	glm::mat3 I(1.0f);

	// Accumulate Jacobians from each spring
	for (auto& spring : springs)
	{
		Particle* p1 = spring->GetEndOne().get();
		Particle* p2 = spring->GetEndTwo().get();

		size_t idx1 = indexMap[p1];
		size_t idx2 = indexMap[p2];

		glm::vec3 diff = p1->GetPosition() - p2->GetPosition();
		float dist = glm::length(diff);
		if (dist < 1e-8f) continue;

		glm::vec3 dir = diff / dist;
		float l0 = spring->GetRestLength();

		// Jacobian of spring force on particle i w.r.t. x_i:
		// dF/dx = -k * [(1 - l0/r)*I + (l0/r) * dir*dir^T]
		float ratio = l0 / dist;
		glm::mat3 dirOuter = glm::outerProduct(dir, dir);
		glm::mat3 Jx = -springK * ((1.0f - ratio) * I + ratio * dirOuter);

		dFdx[idx1] += Jx;
		dFdx[idx2] += Jx;

		// Jacobian of damping force: dF/dv = -c * I (simplified)
		glm::mat3 Jv = -dampingK * I;
		dFdv[idx1] += Jv;
		dFdv[idx2] += Jv;
	}

	// Implicit solve for spring/damping only:
	// (m*I - dt*dFdv - dt^2*dFdx) * dv = dt*F_sd + dt^2*dFdx*v
	// where F_sd = spring + damping forces (in ForceAccumulated)
	for (size_t i = 0; i < n; i++)
	{
		float mass = particles[i]->GetMass();
		glm::vec3 Fsd = particles[i]->GetForceAccumulated();
		glm::vec3 v = particles[i]->GetVelocity();

		glm::mat3 A = mass * I - dt * dFdv[i] - dt * dt * dFdx[i];
		glm::vec3 b = dt * Fsd + dt * dt * dFdx[i] * v;

		float det = glm::determinant(A);
		glm::vec3 dv;
		if (std::fabs(det) < 1e-12f)
		{
			// Fallback to explicit if singular
			dv = (Fsd / mass) * dt;
		}
		else
		{
			dv = glm::inverse(A) * b;
		}

		glm::vec3 newVel = v + dv;
		glm::vec3 newPos = particles[i]->GetPosition() + newVel * dt;

		particles[i]->SetVelocity(newVel);
		particles[i]->SetPosition(newPos);
	}
}

// Paper Section 3.2.4, Eq. 8: Point vs AABB collision + response
void PhysicsEngine::ResolveCollisions(std::vector<std::shared_ptr<Particle>>& particles,
									  const ColliderBox& collider)
{
	if (!collider.enabled) return;

	for (auto& particle : particles)
		collider.ResolveCollision(particle);
}

void PhysicsEngine::ClearForces(std::vector<std::shared_ptr<Particle>>& particles)
{
	for (auto& p : particles)
		p->ClearForce();
}

glm::vec3 PhysicsEngine::TriangleCrossProduct(const glm::vec3& v1,
											   const glm::vec3& v2,
											   const glm::vec3& v3)
{
	return glm::cross(v2 - v1, v3 - v1);
}

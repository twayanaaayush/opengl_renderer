#include "GameObject.h"

GameObject::GameObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const Transform& transform)
	:m_Mesh(mesh), m_Transform(transform), m_Material(material), m_Size(1.0f)
{
	CalculateBoundingBox();
}

GameObject::GameObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
	:m_Mesh(mesh), m_Material(material), m_Size(1.0f)
{
	CalculateBoundingBox();
}

GameObject::GameObject(std::shared_ptr<Mesh> mesh):m_Mesh(mesh), m_Size(1.0f)
{
	CalculateBoundingBox();
}

GameObject::~GameObject()
{}

void GameObject::SetMesh(std::shared_ptr<Mesh> mesh)
{
	m_Mesh = mesh;
	CalculateBoundingBox();
}

void GameObject::SetMaterial(std::shared_ptr<Material> material)
{
	m_Material = material;
}

void GameObject::SetTransform(const Transform& transform)
{
	m_Transform = transform;
}

void GameObject::SetSize(float size)
{
	m_Size = size;
	m_Transform.SetScale(glm::vec3(m_Size));
}

void GameObject::CalculateBoundingBox()
{
	const auto& verts = m_Mesh->GetVertices();
	if (verts.empty()) return;

	glm::vec3 vMin = verts[0].Position;
	glm::vec3 vMax = verts[0].Position;

	for (size_t i = 1; i < verts.size(); ++i)
	{
		const glm::vec3& p = verts[i].Position;
		vMin = glm::min(vMin, p);
		vMax = glm::max(vMax, p);
	}

	m_BoundingBox[0] = vMin;
	m_BoundingBox[1] = vMax;
}


void GameObject::Update(bool BEGIN_SIMULATION, const glm::vec3& position)
{
	(*m_Mesh).UpdateBuffers();
	CalculateBoundingBox();

	m_Transform.SetScale(glm::vec3(m_Size));
	m_Transform.SetTranslation(position);
}

void GameObject::Draw()
{
	(*m_Mesh).Draw();
}

#include "Mesh.h"
#include <glad/glad.h>

Mesh::Mesh()
{
	auto icosphereIndexedMesh = MakeIcosphere(2);
	SetVertices(icosphereIndexedMesh.first);
	SetIndices(icosphereIndexedMesh.second);

	InitBuffers();
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<Triangle> indices)
	: m_Vertices(vertices), m_Indices(indices)
{
	InitBuffers();
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
           std::vector<TextureInfo> textures)
	: m_Vertices(std::move(vertices)),
	  m_FlatIndices(std::move(indices)),
	  m_Textures(std::move(textures))
{
	InitBuffers();
}

Mesh::~Mesh()
{
	delete m_VAO;
	delete m_VBO;
	delete m_EBO;
	delete m_Layout;
}

void Mesh::SetVertices(std::vector<Vertex> vertices)
{
	m_Vertices = vertices;
}

void Mesh::SetIndices(std::vector<Triangle> indices)
{
	m_Indices = indices;
}

void Mesh::InitBuffers()
{
	m_VAO = new VertexArray();
	m_VBO = new VertexBuffer(&m_Vertices[0], (unsigned int)(m_Vertices.size() * sizeof(Vertex)));

	// Use flat indices if available (model loading path), else Triangle-based indices
	if (!m_FlatIndices.empty())
	{
		m_EBO = new IndexBuffer(m_FlatIndices.data(), (unsigned int)m_FlatIndices.size());
	}
	else
	{
		m_EBO = new IndexBuffer(reinterpret_cast<unsigned int*>(&m_Indices[0]),
		                        (unsigned int)(m_Indices.size() * Triangle::GetVertexCount()));
	}

	m_Layout = new BufferLayout();
	m_Layout->Add<float>(3); // Position
	m_Layout->Add<float>(3); // Normal
	m_Layout->Add<float>(2); // TexCoords

	m_VAO->AddBuffer(*m_VBO, *m_Layout);
	m_VAO->Unbind();
}

void Mesh::UpdateBuffers()
{
	m_VBO->Update(&m_Vertices[0], (unsigned int)(m_Vertices.size() * sizeof(Vertex)));
}

void Mesh::Draw() const
{
	m_VAO->Bind();
	m_EBO->Bind();

	unsigned int count = !m_FlatIndices.empty()
		? (unsigned int)m_FlatIndices.size()
		: (unsigned int)(m_Indices.size() * Triangle::GetVertexCount());
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
}

void Mesh::Draw(Shader& shader) const
{
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;

	for (unsigned int i = 0; i < m_Textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);

		std::string number;
		const std::string& name = m_Textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++);

		shader.SetUniform1i(name + number, i);
		glBindTexture(GL_TEXTURE_2D, m_Textures[i].id);
	}

	shader.SetUniform1i("hasTextures", !m_Textures.empty() ? 1 : 0);
	glActiveTexture(GL_TEXTURE0);

	m_VAO->Bind();
	m_EBO->Bind();

	unsigned int count = !m_FlatIndices.empty()
		? (unsigned int)m_FlatIndices.size()
		: (unsigned int)(m_Indices.size() * Triangle::GetVertexCount());
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
	m_VAO->Unbind();
}

Index Mesh::vertex_for_edge(Lookup& lookup, std::vector<Vertex>& vertices, Index first, Index second)
{
	Lookup::key_type key(first, second);

	if (key.first > key.second) std::swap(key.first, key.second);

	auto inserted = lookup.insert({ key, vertices.size() });
	if (inserted.second)
	{
		auto& edge0 = vertices[first];
		auto& edge1 = vertices[second];
		auto point = glm::normalize(edge0.Position + edge1.Position);
		vertices.push_back({ point, glm::vec3(0.0f), glm::vec2(0.0f) });
	}

	return inserted.first->second;
}

std::vector<Triangle> Mesh::Subdivide(std::vector<Vertex>& vertices, std::vector<Triangle> triangles)
{
	Lookup lookup;
	std::vector<Triangle> result;

	for (auto&& each : triangles)
	{
		std::array<Index, 3> mid;
		for (int edge = 0; edge < 3; ++edge)
		{
			mid[edge] = vertex_for_edge(lookup, vertices,
				each.vertex[edge], each.vertex[(edge + 1) % 3]);
		}

		result.push_back({ each.vertex[0], mid[0], mid[2] });
		result.push_back({ each.vertex[1], mid[1], mid[0] });
		result.push_back({ each.vertex[2], mid[2], mid[1] });
		result.push_back({ mid[0], mid[1], mid[2] });
	}

	return result;
}

IndexedMesh Mesh::MakeIcosphere(int subdivisions)
{
	std::vector<Vertex> vertices = icosahedron::vertices;
	std::vector<Triangle> triangles = icosahedron::triangles;

	for (int i = 0; i < subdivisions; ++i)
	{
		triangles = Subdivide(vertices, triangles);
	}

	return { vertices, triangles };
}

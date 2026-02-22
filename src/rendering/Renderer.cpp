#include "Renderer.h"
#include "Softbody.h"
#include "Model.h"
#include "ColliderBox.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(std::shared_ptr<Light> light)
	: m_Light(light)
{
	m_WireframeShader = std::make_shared<Shader>(
		"res/shaders/BasicVertex.shader", "res/shaders/BasicFragment.shader");
	m_ActiveShader = m_WireframeShader;
}

Renderer::~Renderer()
{
	if (m_BoxInitialized)
	{
		glDeleteVertexArrays(1, &m_BoxVAO);
		glDeleteBuffers(1, &m_BoxVBO);
		glDeleteBuffers(1, &m_BoxEBO);
	}
}

void Renderer::SetPhongShader(std::shared_ptr<Shader> shader)
{
	m_SolidShader = shader;
	m_ActiveShader = shader;
}

void Renderer::SetWireframe(bool enabled)
{
	m_ActiveShader = enabled ? m_WireframeShader : m_SolidShader;
}

void Renderer::RenderAll(const std::vector<std::unique_ptr<Softbody>>& objects,
                         Camera& camera, float aspectRatio, float nearPlane, float farPlane)
{
	for (auto& obj : objects)
	{
		if (m_ActiveShader == m_WireframeShader)
		{
			m_WireframeShader->Use();
			m_WireframeShader->SetUniformVec4f("color", glm::vec4(1.0f, 0.5f, 0.31f, 1.0f));
			camera.SetUniforms(*m_ActiveShader, obj->GetTransform().GetModelMatrix(),
			                   aspectRatio, nearPlane, farPlane);
		}
		else
		{
			m_Light->SetUniforms(*m_ActiveShader);
			camera.SetUniformViewPos(*m_ActiveShader);
			obj->GetMaterial().SetUniforms(*m_ActiveShader);
			camera.SetUniforms(*m_ActiveShader, obj->GetTransform().GetModelMatrix(),
			                   aspectRatio, nearPlane, farPlane);
		}

		obj->Draw();
	}
}

void Renderer::RenderModel(Model& model, Camera& camera,
                           float aspectRatio, float nearPlane, float farPlane)
{
	m_SolidShader->Use();
	m_Light->SetUniforms(*m_SolidShader);
	camera.SetUniformViewPos(*m_SolidShader);

	// Set default material for untextured models
	Material defaultMat;
	defaultMat.SetUniforms(*m_SolidShader);

	camera.SetUniforms(*m_SolidShader, model.GetTransform().GetModelMatrix(),
	                   aspectRatio, nearPlane, farPlane);
	model.Draw(*m_SolidShader);
}

void Renderer::InitBoxBuffers()
{
	// Unit cube vertices (8 corners)
	float vertices[] = {
		// positions
		-0.5f, -0.5f, -0.5f,  // 0: left  bottom back
		 0.5f, -0.5f, -0.5f,  // 1: right bottom back
		 0.5f,  0.5f, -0.5f,  // 2: right top    back
		-0.5f,  0.5f, -0.5f,  // 3: left  top    back
		-0.5f, -0.5f,  0.5f,  // 4: left  bottom front
		 0.5f, -0.5f,  0.5f,  // 5: right bottom front
		 0.5f,  0.5f,  0.5f,  // 6: right top    front
		-0.5f,  0.5f,  0.5f   // 7: left  top    front
	};

	// 12 edges as line pairs (24 indices)
	unsigned int indices[] = {
		// back face
		0, 1,  1, 2,  2, 3,  3, 0,
		// front face
		4, 5,  5, 6,  6, 7,  7, 4,
		// connecting edges
		0, 4,  1, 5,  2, 6,  3, 7
	};

	glGenVertexArrays(1, &m_BoxVAO);
	glGenBuffers(1, &m_BoxVBO);
	glGenBuffers(1, &m_BoxEBO);

	glBindVertexArray(m_BoxVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_BoxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BoxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute (location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	m_BoxInitialized = true;
}

// Generic wireframe AABB drawing with configurable color
// parentTransform applies the object's model matrix so the box follows the object
void Renderer::RenderWireBox(const glm::vec3& bbMin, const glm::vec3& bbMax,
                             const glm::vec4& color, Camera& camera,
                             float aspectRatio, float nearPlane, float farPlane,
                             const glm::mat4& parentTransform)
{
	if (!m_BoxInitialized)
		InitBoxBuffers();

	glm::vec3 center = (bbMin + bbMax) * 0.5f;
	glm::vec3 size = bbMax - bbMin;

	// boxModel maps unit cube to local-space AABB,
	// parentTransform then moves it into world space
	glm::mat4 boxModel = glm::mat4(1.0f);
	boxModel = glm::translate(boxModel, center);
	boxModel = glm::scale(boxModel, size);

	glm::mat4 model = parentTransform * boxModel;

	m_WireframeShader->Use();
	m_WireframeShader->SetUniformVec4f("color", color);
	camera.SetUniforms(*m_WireframeShader, model, aspectRatio, nearPlane, farPlane);

	glBindVertexArray(m_BoxVAO);
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

// Render collider box as red wireframe (paper Figure 4)
void Renderer::RenderColliderBox(const ColliderBox& box, Camera& camera,
                                 float aspectRatio, float nearPlane, float farPlane)
{
	RenderWireBox(box.min, box.max, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
	              camera, aspectRatio, nearPlane, farPlane);
}

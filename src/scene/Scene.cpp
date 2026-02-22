#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene(std::shared_ptr<Camera> cam)
{
	InitGrid();
	camera = cam;
}

Scene::~Scene()
{
}

void Scene::InitGrid()
{
	m_GridMesh = std::make_unique<Mesh>(plane::grid, plane::gridIndices);
	m_GridShader = std::make_unique<Shader>("res/shaders/GridVertex.shader", "res/shaders/GridFragment.shader");
}

void Scene::SetGridUniforms(float aspectRatio, float nearPlane, float farPlane)
{
	glm::mat4 projection = glm::perspective(glm::radians(camera->GetZoom()), aspectRatio, nearPlane, farPlane);
	glm::mat4 view = camera->GetViewMatrix();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	m_GridShader->Use();
	m_GridShader->SetUniform1f("nearPlane", nearPlane);
	m_GridShader->SetUniform1f("farPlane", farPlane);
	m_GridShader->SetUniformMat4f("projection", projection);
	m_GridShader->SetUniformMat4f("view", view);
}

void Scene::DrawGrid()
{
	if (m_DrawGrid) m_GridMesh->Draw();
}

void Scene::SetDrawGrid()
{
	m_DrawGrid = !m_DrawGrid;
}

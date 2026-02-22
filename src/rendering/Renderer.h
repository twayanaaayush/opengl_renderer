#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include "Light.h"
#include "Shader.h"
#include "Camera.h"

class Softbody;
class Model;
struct ColliderBox;

class Renderer
{
private:
	std::shared_ptr<Light> m_Light;
	std::shared_ptr<Shader> m_WireframeShader;
	std::shared_ptr<Shader> m_SolidShader;
	std::shared_ptr<Shader> m_ActiveShader;

	// Collider box line rendering resources
	unsigned int m_BoxVAO = 0;
	unsigned int m_BoxVBO = 0;
	unsigned int m_BoxEBO = 0;
	bool m_BoxInitialized = false;

public:
	Renderer(std::shared_ptr<Light> light);
	~Renderer();

	void SetPhongShader(std::shared_ptr<Shader> shader);
	void SetWireframe(bool enabled);
	void RenderAll(const std::vector<std::unique_ptr<Softbody>>& objects,
	               Camera& camera, float aspectRatio, float nearPlane, float farPlane);
	void RenderModel(Model& model, Camera& camera,
	                 float aspectRatio, float nearPlane, float farPlane);
	void RenderColliderBox(const ColliderBox& box, Camera& camera,
	                       float aspectRatio, float nearPlane, float farPlane);
	void RenderWireBox(const glm::vec3& bbMin, const glm::vec3& bbMax,
	                   const glm::vec4& color, Camera& camera,
	                   float aspectRatio, float nearPlane, float farPlane,
	                   const glm::mat4& parentTransform = glm::mat4(1.0f));

private:
	void InitBoxBuffers();
};

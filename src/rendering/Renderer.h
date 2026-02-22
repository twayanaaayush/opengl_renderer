#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include "Light.h"
#include "Shader.h"
#include "Camera.h"

class Softbody;
class Model;

class Renderer
{
private:
	std::shared_ptr<Light> m_Light;
	std::shared_ptr<Shader> m_WireframeShader;
	std::shared_ptr<Shader> m_SolidShader;
	std::shared_ptr<Shader> m_ActiveShader;

public:
	Renderer(std::shared_ptr<Light> light);
	~Renderer();

	void SetPhongShader(std::shared_ptr<Shader> shader);
	void SetWireframe(bool enabled);
	void RenderAll(const std::vector<std::unique_ptr<Softbody>>& objects,
	               Camera& camera, float aspectRatio, float nearPlane, float farPlane);
	void RenderModel(Model& model, Camera& camera,
	                 float aspectRatio, float nearPlane, float farPlane);
};

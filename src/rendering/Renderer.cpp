#include "Renderer.h"
#include "Softbody.h"
#include "Model.h"

Renderer::Renderer(std::shared_ptr<Light> light)
	: m_Light(light)
{
	m_WireframeShader = std::make_shared<Shader>(
		"res/shaders/BasicVertex.shader", "res/shaders/BasicFragment.shader");
	m_ActiveShader = m_WireframeShader;
}

Renderer::~Renderer() {}

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

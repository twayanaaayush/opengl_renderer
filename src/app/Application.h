#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <string>

#include "Camera.h"
#include "Light.h"
#include "Scene.h"
#include "Renderer.h"
#include "Softbody.h"
#include "Model.h"
#include "SimulationParams.h"

class InputHandler;
class ImGuiLayer;
class SimulationUI;

class Application
{
private:
	GLFWwindow* m_Window = nullptr;
	unsigned int m_WindowWidth = 1024;
	unsigned int m_WindowHeight = 780;
	float m_NearPlane = 0.1f;
	float m_FarPlane = 100.0f;

	std::shared_ptr<Camera> m_Camera;
	std::shared_ptr<Light> m_Light;
	std::unique_ptr<Scene> m_Scene;
	std::unique_ptr<Renderer> m_Renderer;
	std::vector<std::unique_ptr<Softbody>> m_Softbodies;
	std::vector<std::unique_ptr<Model>> m_Models;
	std::unique_ptr<InputHandler> m_InputHandler;
	std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
	std::unique_ptr<SimulationUI> m_SimUI;

	SimulationParams m_SimParams;
	bool m_Wireframe = true;
	bool m_SimRunning = false;
	bool m_StepOnce = false;
	bool m_ResetRequested = false;

	double m_DeltaTime = 0.0;
	double m_LastTime = 0.0;

public:
	Application();
	~Application();

	void Run();

	// Called by InputHandler on key presses
	void ToggleWireframe();
	void ToggleSimulation();
	void StepOnce();
	void ResetSimulation();
	void ToggleGrid();

	void LoadModel(const std::string& path);
	void RemoveModel(int index);

	Camera& GetCamera() { return *m_Camera; }
	double GetDeltaTime() const { return m_DeltaTime; }
	const std::vector<std::unique_ptr<Model>>& GetModels() const { return m_Models; }

private:
	bool Init();
	void MainLoop();
	void Shutdown();
	float GetAspectRatio() const;
};

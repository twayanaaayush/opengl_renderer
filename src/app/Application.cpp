#include "Application.h"
#include "InputHandler.h"
#include "ImGuiLayer.h"
#include "SimulationUI.h"
#include "Shader.h"

Application::Application() = default;

Application::~Application()
{
	Shutdown();
}

void Application::Run()
{
	if (!Init()) return;
	MainLoop();
}

bool Application::Init()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_SAMPLES, 4);

	m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, "Soft Body Dynamics", nullptr, nullptr);
	if (!m_Window)
	{
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(m_Window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		return false;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	// Core objects
	m_Camera = std::make_shared<Camera>(glm::vec3(0.0f, 2.5f, 10.0f));
	m_Light = std::make_shared<Light>();

	// Input (must be before ImGui for callback chaining)
	m_InputHandler = std::make_unique<InputHandler>(m_Window, this);

	// Renderer
	m_Renderer = std::make_unique<Renderer>(m_Light);
	auto phongShader = std::make_shared<Shader>(
		"res/shaders/PhongVertex.shader", "res/shaders/PhongFragment.shader");
	m_Renderer->SetPhongShader(phongShader);

	// Scene
	m_Scene = std::make_unique<Scene>(m_Camera);

	// Softbodies
	m_Softbodies.push_back(std::make_unique<Softbody>(0));

	// ImGui (after InputHandler so callback chaining works)
	m_ImGuiLayer = std::make_unique<ImGuiLayer>(m_Window);
	m_SimUI = std::make_unique<SimulationUI>();

	return true;
}

void Application::MainLoop()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		double currentTime = glfwGetTime();
		m_DeltaTime = currentTime - m_LastTime;
		m_LastTime = currentTime;

		m_InputHandler->ProcessInput(m_Window, m_DeltaTime);

		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ImGui new frame
		m_ImGuiLayer->BeginFrame();

		// Draw UI panel
		float fps = (m_DeltaTime > 0) ? static_cast<float>(1.0 / m_DeltaTime) : 0.0f;
		m_SimUI->Draw(m_SimParams, m_SimRunning, m_Wireframe,
		              m_StepOnce, m_ResetRequested, fps, this);

		// Handle reset
		if (m_ResetRequested)
		{
			for (auto& sb : m_Softbodies)
				sb->Reset();
			m_ResetRequested = false;
		}

		// Physics update
		bool shouldSim = m_SimRunning || m_StepOnce;
		for (auto& sb : m_Softbodies)
			sb->Update(shouldSim, m_InputHandler->GetRotXAngle(),
			           m_InputHandler->GetRotYAngle(), m_SimParams);
		if (m_StepOnce) m_StepOnce = false;

		// Render soft bodies
		if (m_Wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		m_Renderer->SetWireframe(m_Wireframe);
		m_Renderer->RenderAll(m_Softbodies, *m_Camera,
		                      GetAspectRatio(), m_NearPlane, m_FarPlane);

		// Render loaded models (always solid)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		for (auto& model : m_Models)
		{
			m_Renderer->RenderModel(*model, *m_Camera,
			                        GetAspectRatio(), m_NearPlane, m_FarPlane);
		}

		// Grid
		m_Scene->SetGridUniforms(GetAspectRatio(), m_NearPlane, m_FarPlane);
		m_Scene->DrawGrid();

		// ImGui render (on top of 3D)
		m_ImGuiLayer->EndFrame();

		glfwSwapBuffers(m_Window);
		glfwPollEvents();
	}
}

void Application::Shutdown()
{
	m_Models.clear();
	m_ImGuiLayer.reset();
	m_SimUI.reset();

	if (m_Window)
	{
		glfwDestroyWindow(m_Window);
		m_Window = nullptr;
	}
	glfwTerminate();
}

float Application::GetAspectRatio() const
{
	return static_cast<float>(m_WindowWidth) / static_cast<float>(m_WindowHeight);
}

void Application::ToggleWireframe()
{
	m_Wireframe = !m_Wireframe;
}

void Application::ToggleSimulation()
{
	m_SimRunning = !m_SimRunning;
}

void Application::StepOnce()
{
	m_StepOnce = true;
}

void Application::ResetSimulation()
{
	m_ResetRequested = true;
}

void Application::ToggleGrid()
{
	m_Scene->SetDrawGrid();
}

void Application::LoadModel(const std::string& path)
{
	auto model = std::make_unique<Model>(path);
	m_Models.push_back(std::move(model));
}

void Application::RemoveModel(int index)
{
	if (index >= 0 && index < static_cast<int>(m_Models.size()))
		m_Models.erase(m_Models.begin() + index);
}

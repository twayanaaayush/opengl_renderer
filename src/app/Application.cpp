#include "Application.h"
#include "InputHandler.h"
#include "ImGuiLayer.h"
#include "SimulationUI.h"
#include "Shader.h"
#include <chrono>
#include <cstdio>

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
		m_SimUI->Draw(m_SimParams, m_SimMetrics, m_SimRunning, m_Wireframe,
		              m_StepOnce, m_ResetRequested, fps, this);

		// Handle reset
		if (m_ResetRequested)
		{
			for (auto& sb : m_Softbodies)
				sb->Reset();
			m_SimMetrics = SimulationMetrics{};
			m_ResetRequested = false;
		}

		// Physics update (timed for metrics)
		bool shouldSim = m_SimRunning || m_StepOnce;
		{
			auto t0 = std::chrono::high_resolution_clock::now();

			for (auto& sb : m_Softbodies)
				sb->Update(shouldSim, m_SimParams, m_SimParams.collider);

			auto t1 = std::chrono::high_resolution_clock::now();
			float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();

			if (shouldSim)
			{
				m_SimMetrics.physicsStepMs = ms;
				// Exponential moving average (α = 0.05)
				m_SimMetrics.avgPhysicsStepMs = m_SimMetrics.avgPhysicsStepMs * 0.95f + ms * 0.05f;
				m_SimMetrics.simFrameCount++;

				// Compute max particle distance from center of mass
				if (!m_Softbodies.empty())
				{
					const glm::vec3* bb = m_Softbodies[0]->GetBoundingBox();
					glm::vec3 center = (bb[0] + bb[1]) * 0.5f;
					float maxDist = 0.0f;

					auto& mesh = m_Softbodies[0]->GetMesh();
					for (const auto& v : mesh.GetVertices())
					{
						float d = glm::length(v.Position - center);
						if (d > maxDist) maxDist = d;
					}
					m_SimMetrics.maxParticleDist = maxDist;

					// Divergence: any particle > 50 units from center
					m_SimMetrics.diverged = (maxDist > 50.0f);
				}
			}
		}
		if (m_StepOnce) m_StepOnce = false;

		// Render soft bodies
		if (m_Wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		m_Renderer->SetWireframe(m_Wireframe);
		m_Renderer->RenderAll(m_Softbodies, *m_Camera,
		                      GetAspectRatio(), m_NearPlane, m_FarPlane);

		// Render bounding boxes and collider box as wireframe lines
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		if (m_SimParams.showBoundingBox)
		{
			for (auto& sb : m_Softbodies)
			{
				const glm::vec3* bb = sb->GetBoundingBox();
				m_Renderer->RenderWireBox(bb[0], bb[1],
				                          glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), // Green
				                          *m_Camera, GetAspectRatio(), m_NearPlane, m_FarPlane,
				                          sb->GetTransform().GetModelMatrix());
			}
		}
		if (m_SimParams.showColliderBox && m_SimParams.collider.enabled)
		{
			m_Renderer->RenderColliderBox(m_SimParams.collider, *m_Camera,
			                              GetAspectRatio(), m_NearPlane, m_FarPlane);
		}

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

void Application::CaptureSnapshot()
{
	const char* methodNames[] = { "Forward Euler", "Midpoint (2nd Order)", "Implicit Euler" };
	const char* method = methodNames[static_cast<int>(m_SimParams.integrationMethod)];

	const char* volMethodNames[] = { "AABB", "Bounding Sphere", "Bounding Ellipsoid", "Exact (Divergence Thm)" };
	const char* volMethod = volMethodNames[static_cast<int>(m_SimParams.volumeMethod)];

	// Soft body metrics
	float volume = 0.0f, pressure = 0.0f;
	float vAABB = 0.0f, vSphere = 0.0f, vEllipsoid = 0.0f, vExact = 0.0f;
	float errAABB = 0.0f, errSphere = 0.0f, errEllipsoid = 0.0f;
	float bbHeight = 0.0f, bbWidth = 0.0f, flatteningRatio = 0.0f;
	float maxDist = 0.0f;
	glm::vec3 centerOfMass(0.0f);
	size_t numParticles = 0, numSprings = 0;

	if (!m_Softbodies.empty())
	{
		auto& sb = m_Softbodies[0];
		const glm::vec3* bb = sb->GetBoundingBox();
		glm::vec3 extent = bb[1] - bb[0];

		volume = sb->GetVolume();
		pressure = sb->GetPressure();
		numParticles = sb->GetParticleCount();
		numSprings = sb->GetSpringCount();

		vAABB = sb->GetVolumeAABB();
		vSphere = sb->GetVolumeSphere();
		vEllipsoid = sb->GetVolumeEllipsoid();
		vExact = sb->GetVolumeExact();

		if (vExact > 1e-6f)
		{
			errAABB = ((vAABB - vExact) / vExact) * 100.0f;
			errSphere = ((vSphere - vExact) / vExact) * 100.0f;
			errEllipsoid = ((vEllipsoid - vExact) / vExact) * 100.0f;
		}

		bbHeight = extent.y;
		bbWidth = std::max(extent.x, extent.z);
		flatteningRatio = (bbWidth > 1e-6f) ? bbHeight / bbWidth : 0.0f;

		glm::vec3 center = (bb[0] + bb[1]) * 0.5f;
		centerOfMass = center;

		auto& mesh = sb->GetMesh();
		for (const auto& v : mesh.GetVertices())
		{
			float d = glm::length(v.Position - center);
			if (d > maxDist) maxDist = d;
		}
	}

	printf("\n");
	printf("╔══════════════════════════════════════════════════════════╗\n");
	printf("║                  SIMULATION SNAPSHOT                    ║\n");
	printf("╠══════════════════════════════════════════════════════════╣\n");
	printf("║  Parameters                                            ║\n");
	printf("║    Integration:    %-36s  ║\n", method);
	printf("║    Spring k:       %-8.1f                              ║\n", m_SimParams.springConstant);
	printf("║    Damping c:      %-8.2f                              ║\n", m_SimParams.dampingConstant);
	printf("║    Time Step dt:   %-8.4f s                            ║\n", m_SimParams.integrationStep);
	printf("║    Gravity:        %-8.2f m/s^2                        ║\n", m_SimParams.gravityStrength);
	printf("║    Particle Mass:  %-8.3f kg                            ║\n", m_SimParams.particleMass);
	printf("║    Moles (nRT):    %-8u                              ║\n", m_SimParams.moles);
	printf("║    Particles:      %-8zu                              ║\n", numParticles);
	printf("║    Springs:        %-8zu                              ║\n", numSprings);
	printf("╠══════════════════════════════════════════════════════════╣\n");
	printf("║  Timing                                                ║\n");
	printf("║    Sim Frame:      %-8d                              ║\n", m_SimMetrics.simFrameCount);
	printf("║    Physics Step:   %-8.3f ms                           ║\n", m_SimMetrics.physicsStepMs);
	printf("║    Avg Step:       %-8.3f ms                           ║\n", m_SimMetrics.avgPhysicsStepMs);
	printf("╠══════════════════════════════════════════════════════════╣\n");
	printf("║  Stability                                             ║\n");
	printf("║    Max Particle Dist: %-8.3f                           ║\n", maxDist);
	printf("║    Diverged:       %-8s                              ║\n", m_SimMetrics.diverged ? "YES" : "NO");
	printf("╠══════════════════════════════════════════════════════════╣\n");
	printf("║  Volume Comparison (Addition 1)                        ║\n");
	printf("║    Active Method:  %-36s  ║\n", volMethod);
	printf("║    V_exact:        %-10.4f                            ║\n", vExact);
	printf("║    V_AABB:         %-10.4f  (err: %+.1f%%)             ║\n", vAABB, errAABB);
	printf("║    V_sphere:       %-10.4f  (err: %+.1f%%)             ║\n", vSphere, errSphere);
	printf("║    V_ellipsoid:    %-10.4f  (err: %+.1f%%)             ║\n", vEllipsoid, errEllipsoid);
	printf("╠══════════════════════════════════════════════════════════╣\n");
	printf("║  Deformation                                           ║\n");
	printf("║    Active Volume:  %-8.4f                              ║\n", volume);
	printf("║    Pressure:       %-8.2f                              ║\n", pressure);
	printf("║    BB Height (H):  %-8.4f                              ║\n", bbHeight);
	printf("║    BB Width  (W):  %-8.4f                              ║\n", bbWidth);
	printf("║    Flattening (H/W): %-8.4f                            ║\n", flatteningRatio);
	printf("║    Center of Mass: (%.2f, %.2f, %.2f)                  ║\n",
		centerOfMass.x, centerOfMass.y, centerOfMass.z);
	printf("╚══════════════════════════════════════════════════════════╝\n");
	printf("\n");

	// CSV-friendly one-liner for easy table building
	printf("CSV: %s, %s, %.3f, %.1f, %.2f, %.4f, %u, %d, %.3f, %.3f, %.3f, %s, %.4f, %.4f, %.4f, %.4f, %.1f, %.1f, %.1f, %.2f, %.4f, %.4f, %.4f\n",
		method, volMethod, m_SimParams.particleMass,
		m_SimParams.springConstant, m_SimParams.dampingConstant,
		m_SimParams.integrationStep, m_SimParams.moles,
		m_SimMetrics.simFrameCount, m_SimMetrics.physicsStepMs,
		m_SimMetrics.avgPhysicsStepMs, maxDist,
		m_SimMetrics.diverged ? "YES" : "NO",
		vExact, vAABB, vSphere, vEllipsoid,
		errAABB, errSphere, errEllipsoid,
		pressure, bbHeight, bbWidth, flatteningRatio);
	printf("CSV Headers: Method, VolMethod, particle_mass, k, c, dt, moles, frames, step_ms, avg_ms, max_dist, diverged, V_exact, V_AABB, V_sphere, V_ellipsoid, err_AABB%%, err_sphere%%, err_ellipsoid%%, pressure, height, width, flattening\n");
	printf("\n");

	fflush(stdout);
}


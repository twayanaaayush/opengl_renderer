#include "InputHandler.h"
#include "Application.h"
#include "Camera.h"
#include "SimulationParams.h"
#include "Softbody.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <limits>

InputHandler::InputHandler(GLFWwindow* window, Application* app)
	: m_App(app)
{
	glfwSetWindowUserPointer(window, this);

	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPosCallback(window, CursorCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
}

// --- Ray helpers ---

glm::vec3 InputHandler::ScreenToRay(GLFWwindow* window, double mouseX, double mouseY, Application* app)
{
	int winW, winH;
	glfwGetWindowSize(window, &winW, &winH);
	if (winW <= 0 || winH <= 0) return glm::vec3(0.0f, 0.0f, -1.0f);

	float ndcX = (2.0f * static_cast<float>(mouseX) / winW) - 1.0f;
	float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY) / winH);

	Camera& cam = app->GetCamera();
	float aspectRatio = static_cast<float>(winW) / static_cast<float>(winH);
	glm::mat4 projection = glm::perspective(glm::radians(cam.GetZoom()), aspectRatio, 0.1f, 100.0f);
	glm::mat4 view = cam.GetViewMatrix();
	glm::mat4 invVP = glm::inverse(projection * view);

	glm::vec4 worldPos = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
	worldPos /= worldPos.w;

	return glm::normalize(glm::vec3(worldPos) - cam.GetPosition());
}

bool InputHandler::RayAABB(const glm::vec3& origin, const glm::vec3& dir,
                            const glm::vec3& bmin, const glm::vec3& bmax, float& tOut)
{
	float tmin = -std::numeric_limits<float>::max();
	float tmax = std::numeric_limits<float>::max();

	for (int i = 0; i < 3; i++)
	{
		if (std::fabs(dir[i]) < 1e-8f)
		{
			if (origin[i] < bmin[i] || origin[i] > bmax[i])
				return false;
		}
		else
		{
			float t1 = (bmin[i] - origin[i]) / dir[i];
			float t2 = (bmax[i] - origin[i]) / dir[i];
			if (t1 > t2) std::swap(t1, t2);
			tmin = std::max(tmin, t1);
			tmax = std::min(tmax, t2);
			if (tmin > tmax) return false;
		}
	}
	tOut = tmin >= 0.0f ? tmin : tmax;
	return tmax >= 0.0f;
}

bool InputHandler::RayPlane(const glm::vec3& origin, const glm::vec3& dir,
                             const glm::vec3& planePoint, const glm::vec3& planeNormal,
                             glm::vec3& hitPoint)
{
	float denom = glm::dot(planeNormal, dir);
	if (std::fabs(denom) < 1e-8f) return false;
	float t = glm::dot(planePoint - origin, planeNormal) / denom;
	if (t < 0.0f) return false;
	hitPoint = origin + dir * t;
	return true;
}

// --- Input handling ---

void InputHandler::ProcessInput(GLFWwindow* window, double deltaTime)
{
	// Only block WASD when a text input is active (e.g. model path),
	// not when any ImGui window is focused (which blocks after any click)
	if (ImGui::GetIO().WantTextInput) return;

	Camera& cam = m_App->GetCamera();

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cam.ProcessKeyboard(Cam::Camera_Movement::FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam.ProcessKeyboard(Cam::Camera_Movement::BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam.ProcessKeyboard(Cam::Camera_Movement::LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam.ProcessKeyboard(Cam::Camera_Movement::RIGHT, deltaTime);
}

void InputHandler::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	auto* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
	if (!handler) return;

	if (ImGui::GetIO().WantCaptureKeyboard) return;

	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
		if (key == GLFW_KEY_E) handler->m_App->ToggleWireframe();
		if (key == GLFW_KEY_Q || key == GLFW_KEY_SPACE) handler->m_App->ToggleSimulation();
		if (key == GLFW_KEY_R) handler->m_App->GetCamera().ResetPosition();
		if (key == GLFW_KEY_G) handler->m_App->ToggleGrid();
		if (key == GLFW_KEY_N) handler->m_App->StepOnce();
		if (key == GLFW_KEY_BACKSPACE) handler->m_App->ResetSimulation();
		if (key == GLFW_KEY_P) handler->m_App->CaptureSnapshot();
	}
}

void InputHandler::CursorCallback(GLFWwindow* window, double xpos, double ypos)
{
	auto* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
	if (!handler) return;

	// Handle active viewport drag (continues even if cursor passes over ImGui panel)
	if (handler->m_DragTarget != DragTarget::None)
	{
		Camera& cam = handler->m_App->GetCamera();
		glm::vec3 rayOrigin = cam.GetPosition();
		glm::vec3 rayDir = ScreenToRay(window, xpos, ypos, handler->m_App);

		glm::vec3 hitPoint;
		if (RayPlane(rayOrigin, rayDir, handler->m_DragPlanePoint,
		             handler->m_DragPlaneNormal, hitPoint))
		{
			SimulationParams& params = handler->m_App->GetSimParams();
			glm::vec3 newPos = hitPoint - handler->m_DragOffset;

			if (handler->m_DragTarget == DragTarget::Object)
			{
				params.objectPosition = newPos;
			}
			else if (handler->m_DragTarget == DragTarget::Collider)
			{
				glm::vec3 delta = newPos - params.colliderPosition;
				params.collider.min += delta;
				params.collider.max += delta;
				params.colliderPosition = newPos;
			}
		}

		handler->m_LastX = xpos;
		handler->m_LastY = ypos;
		return;
	}

	if (ImGui::GetIO().WantCaptureMouse)
	{
		handler->m_LastX = xpos;
		handler->m_LastY = ypos;
		return;
	}

	double xoffset = xpos - handler->m_LastX;
	double yoffset = handler->m_LastY - ypos;

	Camera& cam = handler->m_App->GetCamera();

	if (handler->m_ButtonDown == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
		             glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		bool ctrl  = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
		             glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

		if (shift)
			cam.ProcessMousePan(xoffset, yoffset, handler->m_App->GetDeltaTime());
		else if (ctrl)
			cam.ProcessMouseScroll(yoffset * 0.5);
		else
			cam.ProcessMouseMovement(xoffset, yoffset, handler->m_App->GetDeltaTime());
	}

	handler->m_LastX = xpos;
	handler->m_LastY = ypos;
}

void InputHandler::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	auto* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
	if (!handler) return;

	if (ImGui::GetIO().WantCaptureMouse) return;

	handler->m_App->GetCamera().ProcessMouseScroll(yoffset);
}

void InputHandler::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
	if (!handler) return;

	// Always clear drag on left button release
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		handler->m_DragTarget = DragTarget::None;
		handler->m_ButtonDown = -1;
		return;
	}

	if (ImGui::GetIO().WantCaptureMouse) return;

	glfwGetCursorPos(window, &handler->m_LastX, &handler->m_LastY);

	if (action == GLFW_PRESS)
	{
		handler->m_ButtonDown = button;

		// Left-click: pick objects in viewport
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			Camera& cam = handler->m_App->GetCamera();
			SimulationParams& params = handler->m_App->GetSimParams();

			glm::vec3 rayOrigin = cam.GetPosition();
			glm::vec3 rayDir = ScreenToRay(window, handler->m_LastX, handler->m_LastY, handler->m_App);

			DragTarget bestTarget = DragTarget::None;
			glm::vec3 bestCenter(0.0f);

			// Check soft body AABB first (priority over collider since it's inside)
			const auto& softbodies = handler->m_App->GetSoftbodies();
			if (!softbodies.empty())
			{
				const glm::vec3* bb = softbodies[0]->GetBoundingBox();
				float size = softbodies[0]->GetSize();
				glm::vec3 worldMin = bb[0] * size + params.objectPosition;
				glm::vec3 worldMax = bb[1] * size + params.objectPosition;

				float t;
				if (RayAABB(rayOrigin, rayDir, worldMin, worldMax, t))
				{
					bestTarget = DragTarget::Object;
					bestCenter = (worldMin + worldMax) * 0.5f;
				}
			}

			// Check collider AABB only if soft body wasn't hit
			if (bestTarget == DragTarget::None && params.collider.enabled)
			{
				float t;
				if (RayAABB(rayOrigin, rayDir, params.collider.min, params.collider.max, t))
				{
					bestTarget = DragTarget::Collider;
					bestCenter = (params.collider.min + params.collider.max) * 0.5f;
				}
			}

			if (bestTarget != DragTarget::None)
			{
				handler->m_DragTarget = bestTarget;

				// Drag plane perpendicular to camera forward, through object center
				handler->m_DragPlaneNormal = cam.GetFront();
				handler->m_DragPlanePoint = bestCenter;

				// Offset between hit point on plane and object position
				glm::vec3 hitPoint;
				if (RayPlane(rayOrigin, rayDir, bestCenter, handler->m_DragPlaneNormal, hitPoint))
				{
					if (bestTarget == DragTarget::Object)
						handler->m_DragOffset = hitPoint - params.objectPosition;
					else
						handler->m_DragOffset = hitPoint - params.colliderPosition;
				}
			}
		}
	}
	else
	{
		handler->m_ButtonDown = -1;
	}
}

void InputHandler::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	auto* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
	if (handler && width > 0 && height > 0)
		handler->m_App->SetWindowSize(static_cast<unsigned int>(width), static_cast<unsigned int>(height));
}

#include "InputHandler.h"
#include "Application.h"
#include "Camera.h"
#include <imgui.h>

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

void InputHandler::ProcessInput(GLFWwindow* window, double deltaTime)
{
	if (ImGui::GetIO().WantCaptureKeyboard) return;

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
	}
}

void InputHandler::CursorCallback(GLFWwindow* window, double xpos, double ypos)
{
	auto* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
	if (!handler) return;

	if (ImGui::GetIO().WantCaptureMouse)
	{
		handler->m_LastX = xpos;
		handler->m_LastY = ypos;
		return;
	}

	double xoffset = xpos - handler->m_LastX;
	double yoffset = handler->m_LastY - ypos;

	Camera& cam = handler->m_App->GetCamera();

	if (handler->m_ButtonDown == 1)
		cam.ProcessMouseMovement(xoffset, yoffset, handler->m_App->GetDeltaTime());

	if (handler->m_ButtonDown == 0)
	{
		handler->m_RotXAngle += static_cast<float>((ypos - handler->m_LastY) * cam.GetMouseSensitivity());
		handler->m_RotYAngle += static_cast<float>(xoffset * cam.GetMouseSensitivity());
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

	if (ImGui::GetIO().WantCaptureMouse) return;

	glfwGetCursorPos(window, &handler->m_LastX, &handler->m_LastY);

	if (action == GLFW_PRESS)
		handler->m_ButtonDown = button;
	else
		handler->m_ButtonDown = -1;
}

void InputHandler::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

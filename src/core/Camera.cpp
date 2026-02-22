#include <iostream>
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

Camera::Camera(glm::vec3 Position, glm::vec3 up, float Yaw, float Pitch)
		: m_Front(glm::vec3(0.0f, 0.0f, 0.0f)),
		  m_MovementSpeed(Cam::SPEED),
		  m_MouseSensitivity(Cam::SENSITIVITY),
		  m_Zoom(Cam::ZOOM)
{
	m_InitialPos = m_Position = Position;
	m_InitialUp = m_WorldUp = up;
	m_Yaw = Yaw;
	m_Pitch = Pitch;

	UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

void Camera::ResetPosition()
{
	m_Position = m_InitialPos;
	m_WorldUp = m_InitialUp;
	m_Yaw = Cam::YAW;
	m_Pitch = Cam::PITCH;

	UpdateCameraVectors();
}

void Camera::ProcessKeyboard(Cam::Camera_Movement direction, double deltaTime)
{
	float velocity = m_MovementSpeed * deltaTime;

	if (direction == Cam::Camera_Movement::FORWARD)
		m_Position += m_Front * velocity;
	if (direction == Cam::Camera_Movement::BACKWARD)
		m_Position -= m_Front * velocity;
	if (direction == Cam::Camera_Movement::LEFT)
		m_Position -= m_Right * velocity;
	if (direction == Cam::Camera_Movement::RIGHT)
		m_Position += m_Right * velocity;
}

void Camera::ProcessMouseMovement(double xoffset, double yoffset, double deltaTime, bool constrain_Pitch)
{
	xoffset *= m_MouseSensitivity;
	yoffset *= m_MouseSensitivity;

	m_Yaw += xoffset;
	m_Pitch += yoffset;

	if (constrain_Pitch)
	{
		if (m_Pitch > 89.0f)
			m_Pitch = 89.0f;
		if (m_Pitch < -89.0f)
			m_Pitch = -89.0f;
	}

	UpdateCameraVectors();
}

void Camera::ProcessMousePan(double xoffset, double yoffset, double /*deltaTime*/)
{
	float panSpeed = 0.005f * m_MovementSpeed;
	m_Position -= m_Right * static_cast<float>(xoffset) * panSpeed;
	m_Position -= m_Up * static_cast<float>(yoffset) * panSpeed;
}

void Camera::ProcessMouseScroll(double yoffset)
{
	float zoomSpeed = 0.5f;
	m_Position += m_Front * static_cast<float>(yoffset) * zoomSpeed;
}

void Camera::UpdateCameraVectors()
{
	glm::vec3 new_front;

	new_front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	new_front.y = sin(glm::radians(m_Pitch));
	new_front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

	m_Front = glm::normalize(new_front);

	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::SetUniforms(Shader& shader, glm::mat4& transform, float aspectRatio, float nearPlane, float farPlane)
{
	shader.Use();

	glm::mat4 projection = glm::perspective(glm::radians(GetZoom()), aspectRatio, nearPlane, farPlane);
	glm::mat4 view = GetViewMatrix();

	shader.SetUniformMat4f("projection", projection);
	shader.SetUniformMat4f("view", view);
	shader.SetUniformMat4f("model", transform);
}

void Camera::SetUniformViewPos(Shader& shader)
{
	glm::vec3 viewPos = GetPosition();
	shader.SetUniformVec3f("viewPos", viewPos);
}

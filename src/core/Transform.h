#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform
{
private:
	glm::vec3 m_Pos = glm::vec3(0.0f);
	glm::vec3 m_Scale = glm::vec3(1.0f);
	float m_RotateX = 0.0f;
	float m_RotateY = 0.0f;
	float m_RotateZ = 0.0f;
	glm::vec3 m_Translate = glm::vec3(0.0f);
	glm::mat4 m_ModelMatrix = glm::mat4(1.0f);

public:
	Transform(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f));
	Transform(const Transform& other);
	Transform(glm::vec3 pos, glm::vec3 scale, glm::vec3 rotate, glm::vec3 translate);

	void operator = (const Transform& other);

	void ToOrigin();

	void SetScale(glm::vec3 factor);
	void SetRotation(glm::vec3 angle);
	void SetTranslation(glm::vec3 factor);

	void Scale_X(float factor);			void Scale_Y(float factor);			void Scale_Z(float factor);
	void Rotate_X(float angle);			void Rotate_Y(float angle);			void Rotate_Z(float angle);
	void Translate_X(float factor);		void Translate_Y(float factor);		void Translate_Z(float factor);
	
	inline glm::mat4& GetModelMatrix() { return m_ModelMatrix; }
	inline glm::vec3 GetTranslation() const { return m_Translate; }
	inline glm::vec3 GetScale() const { return m_Scale; }
	inline glm::vec3 GetRotation() const { return glm::vec3(m_RotateX, m_RotateY, m_RotateZ); }

private:
	void UpdateModelMatrix();
};
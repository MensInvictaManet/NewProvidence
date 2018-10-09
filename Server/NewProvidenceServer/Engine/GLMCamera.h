/*
tdogl::Camera

Copyright 2012 Thomas Dalling - http://tomdalling.com/

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once

#include <glm/glm.hpp>

namespace tdogl {

	/**
	A first-person shooter type of camera.

	Set the properties of the camera, then use the `matrix` method to get the camera matrix for
	use in the vertex shader.

	Includes the perspective projection matrix.
	*/
	class Camera {
	public:
		Camera();

		/**
		The position of the camera.
		*/
		const glm::vec3& position() const;
		void setPosition(const glm::vec3& position);
		void setRotation(const float horizontal, const float vertical);
		void offsetPosition(const glm::vec3& offset);

		/**
		The vertical viewing angle of the camera, in degrees.

		Determines how "wide" the view of the camera is. Large angles appear to be zoomed out,
		as the camera has a wide view. Small values appear to be zoomed in, as the camera has a
		very narrow view.

		The value must be between 0 and 180.
		*/
		float fieldOfView() const;
		void setFieldOfView(float fieldOfView);

		/**
		The closest visible distance from the camera.

		Objects that are closer to the camera than the near plane distance will not be visible.

		Value must be greater than 0.
		*/
		float nearPlane() const;

		/**
		The farthest visible distance from the camera.

		Objects that are further away from the than the far plane distance will not be visible.

		Value must be greater than the near plane
		*/
		float farPlane() const;

		/**
		Sets the near and far plane distances.

		Everything between the near plane and the var plane will be visible. Everything closer
		than the near plane, or farther than the far plane, will not be visible.

		@param nearPlane  Minimum visible distance from camera. Must be > 0
		@param farPlane   Maximum visible distance from vamera. Must be > nearPlane
		*/
		void setNearAndFarPlanes(float nearPlane, float farPlane);

		/**
		A rotation matrix that determines the direction the camera is looking.

		Does not include translation (the camera's position).
		*/
		glm::mat4 orientation() const;

		/**
		Offsets the cameras orientation.

		The verticle angle is constrained between 85deg and -85deg to avoid gimbal lock.

		@param upAngle     the angle (in degrees) to offset upwards. Negative values are downwards.
		@param rightAngle  the angle (in degrees) to offset rightwards. Negative values are leftwards.
		*/
		void offsetOrientation(float upAngle, float rightAngle);

		/**
		Orients the camera so that is it directly facing `position`

		@param position  the position to look at
		*/
		void lookAt(glm::vec3 position);

		/**
		The width divided by the height of the screen/window/viewport

		Incorrect values will make the 3D scene look stretched.
		*/
		float viewportAspectRatio() const;
		void setViewportAspectRatio(float viewportAspectRatio);

		/** A unit vector representing the direction the camera is facing */
		glm::vec3 forward() const;

		/** A unit vector representing the direction to the right of the camera*/
		glm::vec3 right() const;

		/** A unit vector representing the direction out of the top of the camera*/
		glm::vec3 up() const;

		/**
		The combined camera transformation matrix, including perspective projection.

		This is the complete matrix to use in the vertex shader.
		*/
		glm::mat4 matrix() const;

		/**
		The perspective projection transformation matrix
		*/
		glm::mat4 projection() const;

		/**
		The translation and rotation matrix of the camera.

		Same as the `matrix` method, except the return value does not include the projection
		transformation.
		*/
		glm::mat4 view() const;

		void RotateWorld(float angle, glm::vec3 rot);
		void ApplyTransform(bool bRotationOnly = false) const;

	private:
		glm::vec3 _position;
		float _horizontalAngle;
		float _verticalAngle;
		float _fieldOfView;
		float _nearPlane;
		float _farPlane;
		float _viewportAspectRatio;

		void normalizeAngles();
	};

}

/*
tdogl::Camera

Copyright 2012 Thomas Dalling - http://tomdalling.com/

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
//#define _USE_MATH_DEFINES
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

using namespace tdogl;

static const float MaxVerticalAngle = 85.0f; //must be less than 90 to avoid gimbal lock

Camera::Camera() :
	_position(0.0f, 0.0f, 1.0f),
	_horizontalAngle(0.0f),
	_verticalAngle(0.0f),
	_fieldOfView(45.0f),
	_nearPlane(0.01f),
	_farPlane(1000.0f),
	_viewportAspectRatio(4.0f / 3.0f)
{
}

const glm::vec3& Camera::position() const {
	return _position;
}

void Camera::setPosition(const glm::vec3& position) {
	_position = position;
}

void Camera::setRotation(const float horizontal, const float vertical) {
	_horizontalAngle = horizontal;
	_verticalAngle = vertical;
}

void Camera::offsetPosition(const glm::vec3& offset) {
	_position += offset;
}

float Camera::fieldOfView() const {
	return _fieldOfView;
}

void Camera::setFieldOfView(float fieldOfView) {
	assert(fieldOfView > 0.0f && fieldOfView < 180.0f);
	_fieldOfView = fieldOfView;
}

float Camera::nearPlane() const {
	return _nearPlane;
}

float Camera::farPlane() const {
	return _farPlane;
}

void Camera::setNearAndFarPlanes(float nearPlane, float farPlane) {
	assert(nearPlane > 0.0f);
	assert(farPlane > nearPlane);
	_nearPlane = nearPlane;
	_farPlane = farPlane;
}

glm::mat4 Camera::orientation() const {
	glm::mat4 orientation;
	orientation = glm::rotate(orientation, glm::radians(_verticalAngle), glm::vec3(1, 0, 0));
	orientation = glm::rotate(orientation, glm::radians(_horizontalAngle), glm::vec3(0, 1, 0));
	return orientation;
}

void Camera::offsetOrientation(float upAngle, float rightAngle) {
	_horizontalAngle += rightAngle;
	_verticalAngle += upAngle;
	normalizeAngles();
}

void Camera::lookAt(glm::vec3 position) {
	assert(position != _position);
	glm::vec3 direction = glm::normalize(position - _position);
	_verticalAngle = glm::radians(asinf(-direction.y));
	_horizontalAngle = -glm::radians(atan2f(-direction.x, -direction.z));
	normalizeAngles();
}

float Camera::viewportAspectRatio() const {
	return _viewportAspectRatio;
}

void Camera::setViewportAspectRatio(float viewportAspectRatio) {
	assert(viewportAspectRatio > 0.0);
	_viewportAspectRatio = viewportAspectRatio;
}

glm::vec3 Camera::forward() const {
	glm::vec4 forward = glm::inverse(orientation()) * glm::vec4(0, 0, -1, 1);
	return glm::vec3(forward);
}

glm::vec3 Camera::right() const {
	glm::vec4 right = glm::inverse(orientation()) * glm::vec4(1, 0, 0, 1);
	return glm::vec3(right);
}

glm::vec3 Camera::up() const {
	glm::vec4 up = glm::inverse(orientation()) * glm::vec4(0, 1, 0, 1);
	return glm::vec3(up);
}

glm::mat4 Camera::matrix() const {
	return projection() * view();
}

glm::mat4 Camera::projection() const {
	return glm::perspective(glm::radians(_fieldOfView), _viewportAspectRatio, _nearPlane, _farPlane);
}

glm::mat4 Camera::view() const {
	return orientation() * glm::translate(glm::mat4(), -_position);
}

void Camera::normalizeAngles() {
	_horizontalAngle = fmodf(_horizontalAngle, 360.0f);
	//fmodf can return negative values, but this will make them all positive
	if (_horizontalAngle < 0.0f)
		_horizontalAngle += 360.0f;

	if (_verticalAngle > MaxVerticalAngle)
		_verticalAngle = MaxVerticalAngle;
	else if (_verticalAngle < -MaxVerticalAngle)
		_verticalAngle = -MaxVerticalAngle;
}

static void RotateMatrix(float* matrix, float angle, glm::vec3 rot)
{
	float mag, s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = (float)sin(angle);
	c = (float)cos(angle);

	mag = (float)sqrt(rot.x * rot.x + rot.y * rot.y + rot.z * rot.z);

	// Identity matrix
	if (mag == 0.0f)
	{
		matrix[0] = 1.0f;
		matrix[1] = 0.0f;
		matrix[2] = 0.0f;
		matrix[3] = 0.0f;
		matrix[4] = 0.0f;
		matrix[5] = 1.0f;
		matrix[6] = 0.0f;
		matrix[7] = 0.0f;
		matrix[8] = 0.0f;
		matrix[9] = 0.0f;
		matrix[10] = 1.0f;
		matrix[11] = 0.0f;
		matrix[12] = 0.0f;
		matrix[13] = 0.0f;
		matrix[14] = 0.0f;
		matrix[15] = 1.0f;
		return;
	}

	// Normalize the Rotation Matrix
	rot.x /= mag;
	rot.y /= mag;
	rot.z /= mag;

#define M(row,col)  matrix[col*4+row]

	xx = rot.x * rot.x;
	yy = rot.y * rot.y;
	zz = rot.z * rot.z;
	xy = rot.x * rot.y;
	yz = rot.y * rot.z;
	zx = rot.z * rot.x;
	xs = rot.x * rot.s;
	ys = rot.y * rot.s;
	zs = rot.z * rot.s;
	one_c = 1.0f - c;

	M(0, 0) = (one_c * xx) + c;
	M(0, 1) = (one_c * xy) - zs;
	M(0, 2) = (one_c * zx) + ys;
	M(0, 3) = 0.0f;

	M(1, 0) = (one_c * xy) + zs;
	M(1, 1) = (one_c * yy) + c;
	M(1, 2) = (one_c * yz) - xs;
	M(1, 3) = 0.0f;

	M(2, 0) = (one_c * zx) - ys;
	M(2, 1) = (one_c * yz) + xs;
	M(2, 2) = (one_c * zz) + c;
	M(2, 3) = 0.0f;

	M(3, 0) = 0.0f;
	M(3, 1) = 0.0f;
	M(3, 2) = 0.0f;
	M(3, 3) = 1.0f;

#undef M
}

void Camera::RotateWorld(float angle, glm::vec3 rot)
{
	float matrix[16];

	RotateMatrix(matrix, angle, rot);

	glm::vec3 vNewVect;

	// Transform the up axis (inlined 3x3 rotation)
	vNewVect.x = matrix[0] * up().x + matrix[4] * up().y + matrix[8] * up().z;
	vNewVect.y = matrix[1] * up().x + matrix[5] * up().y + matrix[9] * up().z;
	vNewVect.z = matrix[2] * up().x + matrix[6] * up().y + matrix[10] * up().z;
	//up() = vNewVect;

	// Transform the forward axis
	vNewVect.x = matrix[0] * forward().x + matrix[4] * forward().y + matrix[8] * forward().z;
	vNewVect.y = matrix[1] * forward().x + matrix[5] * forward().y + matrix[9] * forward().z;
	vNewVect.z = matrix[2] * forward().x + matrix[6] * forward().y + matrix[10] * forward().z;
	//forward() = vNewVect;
}

static void vec3_cross_product(glm::vec3& out, glm::vec3 in1, glm::vec3 in2)
{
	out.x = in1.y * in2.z - in1.z * in2.y;
	out.y = in1.z * in2.x - in1.x * in2.z;
	out.z = in1.x * in2.y - in1.y * in2.x;
}

void Camera::ApplyTransform(bool bRotationOnly) const {
	const glm::vec3 Z = forward() * -1.0f;
	glm::vec3 X;
	vec3_cross_product(X, up(), Z);

	float matrixFloats[16] =
	{
		X.x,		up().x,		Z.x,		0.0f,
		X.y,		up().y,		Z.y,		0.0f,
		X.z,		up().z,		Z.z,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	};

	glMultMatrixf(matrixFloats);

	if (bRotationOnly == false) glTranslatef(-_position.x, -_position.y, -_position.z);
}

tdogl::Camera gCamera;
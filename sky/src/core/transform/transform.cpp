#include "transform.h"

namespace sky
{
Transform::Transform() :
    m_position(0.0f, 0.0f, 0.0f), 
    m_rotationQuat(glm::identity<glm::quat>()), 
    m_scale(1.0f, 1.0f, 1.0f)
{}
} // namespace sky
#pragma once

#include <skypch.h>

#include <glm/glm.hpp>

namespace sky
{
namespace physics
{
enum class RigidBodyShapes
{
    BOX = 0,
    SPHERE,
    CAPSULE,
    MESH,
    CYLINDER,
    CONVEX_HULL
};

class PhysicShape
{
  protected:
    RigidBodyShapes m_type;

  public:
    [[nodiscard]] RigidBodyShapes getType() const { return m_type; }
};

class Box : public PhysicShape
{
  public:
    Box() : m_size(glm::vec3(1.0f)) { m_type = RigidBodyShapes::BOX; };
    Box(glm::vec3 size) : m_size(size) { m_type = RigidBodyShapes::BOX; };
    Box(float x, float y, float z) : m_size(glm::vec3(x, y, z)) { m_type = RigidBodyShapes::BOX; };

    [[nodiscard]] glm::vec3 getSize() const { return m_size; }

  private:
    glm::vec3 m_size;
};

class Sphere : public PhysicShape
{
  public:
    Sphere(float radius) : m_radius(radius) { m_type = RigidBodyShapes::SPHERE; };

    [[nodiscard]] float getRadius() const { return m_radius; }
    void setRadius(float radius) { radius = radius; };

  private:
    float m_radius;
};

class Capsule : public PhysicShape
{
  public:
    Capsule() = default;

    Capsule(float radius, float height) : m_radius(radius), m_height(height) { m_type = RigidBodyShapes::CAPSULE; }

    [[nodiscard]] float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = radius; }
    [[nodiscard]] float getHeight() const { return m_height; }
    void setHeight(float height) { m_height = height; }

  private:
    float m_radius;
    float m_height;
};

class Cylinder : public PhysicShape
{
  public:
    Cylinder(float radius, float height) : m_radius(radius), m_height(height) { m_type = RigidBodyShapes::CYLINDER; }

    [[nodiscard]] float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = radius; }
    [[nodiscard]] float getHeight() const { return m_height; }
    void setHeight(float height) { m_height = height; }

  private:
    float m_radius;
    float m_height;
};

class MeshShape : public PhysicShape
{
};

class ConvexHullShape : public PhysicShape
{ 
  public:
    ConvexHullShape(const std::vector<glm::vec3> &points) : m_points(points) { m_type = RigidBodyShapes::CONVEX_HULL; }

    [[nodiscard]] std::vector<glm::vec3> getPoints() const { return m_points; }
    void setPoints(const std::vector<glm::vec3> &points) { m_points = points; }

  private:
    std::vector<glm::vec3> m_points;
};
} // namespace physics
} // namespace sky
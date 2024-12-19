#include "math.h"

#include <glm/gtx/norm.hpp> // length2, dot

namespace sky::math
{
void DecomposeMatrix(const glm::mat4 &matrix, glm::vec3 &translation, glm::vec3 &rotation, glm::vec3 &scale)
{
    /*	Keep in mind that the resulting quaternion in not correct. It returns its conjugate!
            To fix this add this to your code:
            rotation = glm::conjugate(rotation);
    */
    glm::quat r;
    glm::vec3 s;
    glm::vec4 p;
    glm::decompose(matrix, scale, r, translation, s, p);
    rotation = glm::eulerAngles(r);
}

void DecomposeMatrix(const glm::mat4 &matrix, glm::vec3 &translation, glm::quat &rotation, glm::vec3 &scale)
{
    /*	Keep in mind that the resulting quaternion in not correct. It returns its conjugate!
            To fix this add this to your code:
            rotation = glm::conjugate(rotation);
    */
    glm::quat r;
    glm::vec3 s;
    glm::vec4 p;
    glm::decompose(matrix, scale, rotation, translation, s, p);
}

float Lerp(float a, float b, float f) { return a + f * (b - a); }

int BinomialCoefficient(int n, int k)
{
    if (k > n) return 0;
    if (k == 0 || k == n) return 1;
    int c = 1;
    for (int i = 0; i < k; ++i)
    {
        c = c * (n - i) / (i + 1);
    }
    return c;
}

float CalculateBezierFactor(float currentTime, float start, float end, float blendMin, float blendMax,
                            const std::vector<float> &points)
{
    float u = glm::clamp((currentTime - start) / (end - start), 0.0f, 1.0f);
    float v = 1.0f - u;
    int n = points.size() + 1;

    float blendFactor = glm::pow(v, n) * blendMin;

    for (int i = 0; i < points.size(); ++i)
    {
        int coeff = BinomialCoefficient(n, i + 1);
        blendFactor += coeff * glm::pow(v, n - i - 1) * glm::pow(u, i + 1) * points[i];
    }

    blendFactor += glm::pow(u, n) * blendMax;

    return blendFactor;

    // float u = glm::clamp((currentTime - start) / (end - start), 0.0f, 1.0f);
    // float v = 1.0f - u;
    // int n = points.size() + 1;

    //// ���� ����� 4 ����� (���������� ������ �����)
    // if (points.size() == 4) {
    //	return (v * v * v * v * v) * blendMin +
    //		(5 * v * v * v * v * u) * points[0] +
    //		(10 * v * v * v * u * u) * points[1] +
    //		(10 * v * v * u * u * u) * points[2] +
    //		(5 * v * u * u * u * u) * points[3] +
    //		(u * u * u * u * u) * blendMax;
    // }

    //// ����� ������ ��� ������ ����� ������ �������
    // float blendFactor = glm::pow(v, n) * blendMin;

    // for (int i = 0; i < points.size(); ++i)
    //{
    //	int coeff = BinomialCoefficient(n, i + 1);
    //	blendFactor += coeff * glm::pow(v, n - i - 1) * glm::pow(u, i + 1) * points[i];
    // }

    // blendFactor += glm::pow(u, n) * blendMax;

    // return blendFactor;
}

std::vector<glm::vec2> FindThree(const std::vector<glm::vec2> &points, const std::vector<float> &weights)
{
    // ������ ��� �������� �������� ���� ����� � ����������� ������
    std::vector<size_t> topIndices(3);

    // ������ ��� �������� �������� �����
    std::vector<size_t> indices(points.size());
    std::iota(indices.begin(), indices.end(), 0);

    // ������� ���-3 ������� � ����������� ������
    std::nth_element(indices.begin(), indices.begin() + 3, indices.end(),
                     [&](size_t a, size_t b) { return weights[a] > weights[b]; });

    //// ��������� ���-3 ������� ��� ����������� �������
    // std::sort(indices.begin(), indices.begin() + 3,
    //	[&](size_t a, size_t b) { return weights[a] > weights[b]; });

    // �������� ���-3 �����
    std::vector<glm::vec2> topThree;
    for (size_t i = 0; i < 3; ++i)
    {
        topThree.push_back(points[indices[i]]);
    }
    return topThree;
}

std::array<std::size_t, 3> FindTree(const std::vector<glm::vec2> &points, const std::vector<float> &weights)
{
    std::vector<std::pair<float, std::size_t>> value_index_pairs;

    for (std::size_t i = 0; i < weights.size(); ++i) value_index_pairs.emplace_back(weights[i], i);

    std::sort(value_index_pairs.begin(), value_index_pairs.end(),
              [](const auto &a, const auto &b) { return a.first > b.first; });

    std::array<size_t, 3> indices = {value_index_pairs[0].second, value_index_pairs[1].second,
                                     value_index_pairs[2].second};

    std::sort(indices.begin(), indices.end(), [](const auto &a, const auto &b) { return a < b; });

    return indices;
}

glm::vec3 ComputeBarycentricCoordinates(const glm::vec2 &p0, const glm::vec2 &p1, const glm::vec2 &p2,
                                        const glm::vec2 &sample_point)
{

    glm::vec2 v0 = p1 - p0;
    glm::vec2 v1 = p2 - p0;
    glm::vec2 v2 = sample_point - p0;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}

std::array<std::pair<std::size_t, float>, 3> CalculateGeneralizedTriangularWeights(const std::vector<glm::vec2> &points,
                                                                                   const glm::vec2 &sample_point)
{
    assert(points.size() >= 3);

    std::array<std::pair<std::size_t, float>, 3> pointsWithWeight;

    std::vector<float> weights = Calculate2DWeightsPolar(points, sample_point);

    std::array<std::size_t, 3> indices = FindTree(points, weights);

    glm::vec3 barycentric_coords =
        ComputeBarycentricCoordinates(points[indices[0]], points[indices[1]], points[indices[2]], sample_point);

    if (barycentric_coords.x >= 0.0f && barycentric_coords.y >= 0.0f && barycentric_coords.z >= 0.0f)
    {
        pointsWithWeight.at(0) = {indices[0], barycentric_coords.x};
        pointsWithWeight.at(1) = {indices[1], barycentric_coords.y};
        pointsWithWeight.at(2) = {indices[2], barycentric_coords.z};
    }
    else
    {
        pointsWithWeight.at(0) = {indices[0], weights[indices[0]]};
        pointsWithWeight.at(1) = {indices[1], weights[indices[1]]};
        pointsWithWeight.at(2) = {indices[2], weights[indices[2]]};
    }

    // ����������� ���� � ������ �������������

    float totalWeight = pointsWithWeight[0].second + pointsWithWeight[1].second + pointsWithWeight[2].second;

    if (totalWeight > 0.0f)
    {
        for (auto &[i, weight] : pointsWithWeight)
        {
            weight /= totalWeight;

            if (std::isnan(weight) || -std::isnan(weight)) weight = 0.f;
        }
    }

    return pointsWithWeight;
}

std::vector<float> Calculate2DWeightsPolar(const std::vector<glm::vec2> &points, const glm::vec2 &sample_point)
{
    const int POINT_COUNT = static_cast<int>(points.size());
    const float kDirScale = 2.0f;
    std::vector<float> weights(POINT_COUNT, 0.0f);
    float total_weight = 0.0f;

    float sample_mag = glm::length(sample_point);

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        glm::vec2 point_i = points[i];
        float point_mag_i = glm::length(point_i);

        float weight = 1.0f;

        for (int j = 0; j < POINT_COUNT; ++j)
        {
            if (j == i) continue;

            glm::vec2 point_j = points[j];
            float point_mag_j = glm::length(point_j);

            float ij_avg_mag = (point_mag_j + point_mag_i) * 0.5f;

            float mag_is = (sample_mag - point_mag_i) / ij_avg_mag;
            float angle_is = SignedAngle(point_i, sample_point);

            float mag_ij = (point_mag_j - point_mag_i) / ij_avg_mag;
            float angle_ij = SignedAngle(point_i, point_j);

            glm::vec2 vec_is = glm::vec2(mag_is, angle_is * kDirScale);
            glm::vec2 vec_ij = glm::vec2(mag_ij, angle_ij * kDirScale);

            float lensq_ij = glm::dot(vec_ij, vec_ij);
            float new_weight = glm::dot(vec_is, vec_ij) / lensq_ij;
            new_weight = 1.0f - new_weight;
            new_weight = std::clamp(new_weight, 0.0f, 1.0f);

            weight = std::min(new_weight, weight);
        }

        weights[i] = weight;
        total_weight += weight;
    }

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        weights[i] /= total_weight;
        if (std::isnan(weights[i]) || -std::isnan(weights[i])) weights[i] = 0.f;
    }

    return weights;
}

std::vector<float> Calculate2DWeightsCartesian(const std::vector<glm::vec2> &points, const glm::vec2 &sample_point)
{
    const int POINT_COUNT = static_cast<int>(points.size());
    std::vector<float> weights(POINT_COUNT, 0.0f);
    float total_weight = 0.0f;

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        glm::vec2 point_i = points[i];
        glm::vec2 vec_is = sample_point - point_i;

        float weight = 1.0f;

        for (int j = 0; j < POINT_COUNT; ++j)
        {
            if (j == i) continue;

            glm::vec2 point_j = points[j];
            glm::vec2 vec_ij = point_j - point_i;

            float lensq_ij = glm::dot(vec_ij, vec_ij);
            float new_weight = glm::dot(vec_is, vec_ij) / lensq_ij;
            new_weight = 1.0f - new_weight;
            new_weight = std::clamp(new_weight, 0.0f, 1.0f);

            weight = std::min(weight, new_weight);
        }

        weights[i] = weight;
        total_weight += weight;
    }

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        weights[i] /= total_weight;
    }

    return weights;
}

std::vector<float> Calculate2DWeightsGBi(const std::vector<glm::vec2> &points, const glm::vec2 &sample_point)
{
    std::vector<float> weights(points.size(), 0.0f);
    std::vector<float> distances(points.size(), 0.0f);

    float totalInverseDistance = 0.0f;

    for (size_t i = 0; i < points.size(); ++i)
    {
        float distance = glm::distance(points[i], sample_point);
        distances[i] = distance;

        if (distance > 0.0f)
        {
            float inverseDistance = 1.0f / distance;
            totalInverseDistance += inverseDistance;
            distances[i] = inverseDistance;
        }
        else
        {

            weights[i] = 1.0f;
            return weights;
        }
    }

    for (size_t i = 0; i < points.size(); ++i)
    {
        if (distances[i] > 0.0f)
        {
            weights[i] = distances[i] / totalInverseDistance;
        }
        else
        {
            weights[i] = 0.0f;
        }
    }

    return weights;
}

// Adapted from "Foundations of Game Engine Development" Part 2, 9.3 "Bounding Volumes"
math::Sphere calculateBoundingSphere(std::span<glm::vec3> positions)
{
    assert(!positions.empty());
    auto calculateInitialSphere = [](const std::span<glm::vec3> &positions) -> math::Sphere
    {
        constexpr int dirCount = 13;
        static const std::array<glm::vec3, dirCount> direction = {{
            {1.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
            {0.f, 0.f, 1.f},
            {1.f, 1.f, 0.f},
            {1.f, 0.f, 1.f},
            {0.f, 1.f, 1.f},
            {1.f, -1.f, 0.f},
            {1.f, 0.f, -1.f},
            {0.f, 1.f, -1.f},
            {1.f, 1.f, 1.f},
            {1.f, -1.f, 1.f},
            {1.f, 1.f, -1.f},
            {1.f, -1.f, -1.f},
        }};

        std::array<float, dirCount> dmin{};
        std::array<float, dirCount> dmax{};
        std::array<std::size_t, dirCount> imin{};
        std::array<std::size_t, dirCount> imax{};

        // Find min and max dot products for each direction and record vertex indices.
        for (int j = 0; j < dirCount; ++j)
        {
            const auto &u = direction[j];
            dmin[j] = glm::dot(u, positions[0]);
            dmax[j] = dmin[j];
            for (std::size_t i = 1; i < positions.size(); ++i)
            {
                const auto d = glm::dot(u, positions[i]);
                if (d < dmin[j])
                {
                    dmin[j] = d;
                    imin[j] = i;
                }
                else if (d > dmax[j])
                {
                    dmax[j] = d;
                    imax[j] = i;
                }
            };
        }

        // Find direction for which vertices at min and max extents are furthest apart.
        float d2 = glm::length2(positions[imax[0]] - positions[imin[0]]);
        int k = 0;
        for (int j = 1; j < dirCount; j++)
        {
            const auto m2 = glm::length2(positions[imax[j]] - positions[imin[j]]);
            if (m2 > d2)
            {
                d2 = m2;
                k = j;
            }
        }

        const auto center = (positions[imin[k]] + positions[imax[k]]) * 0.5f;
        float radius = sqrt(d2) * 0.5f;
        return {center, radius};
    };

    // Determine initial center and radius.
    auto s = calculateInitialSphere(positions);
    // Make pass through vertices and adjust sphere as necessary.
    for (std::size_t i = 0; i < positions.size(); i++)
    {
        const auto pv = positions[i] - s.center;
        float m2 = glm::length2(pv);
        if (m2 > s.radius * s.radius)
        {
            auto q = s.center - (pv * (s.radius / std::sqrt(m2)));
            s.center = (q + positions[i]) * 0.5f;
            s.radius = glm::length(q - s.center);
        }
    }
    return s;
}

glm::vec3 smoothDamp(const glm::vec3 &current, glm::vec3 target, glm::vec3 &currentVelocity, float smoothTime, float dt,
                     float maxSpeed)
{
    if (glm::length(current - target) < 0.0001f)
    {
        // really close - just return target and stop moving
        currentVelocity = glm::vec3{};
        return target;
    }

    // default smoothing if maxSpeed not passed
    smoothTime = std::max(0.0001f, smoothTime);

    auto change = current - target;

    // limit speed if needed
    float maxChange = maxSpeed * smoothTime;
    float changeSqMag = glm::length2(change);
    if (changeSqMag > maxChange * maxChange)
    {
        const auto mag = std::sqrt(changeSqMag);
        change *= maxChange / mag;
    }

    const auto originalTarget = target;
    target = current - change;

    // Scary math. (critically damped harmonic oscillator.)
    // See Game Programming Gems 4, ch 1.10 for details
    float omega = 2.f / smoothTime;
    float x = omega * dt;
    // approximation of e^x
    float exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);
    const auto temp = (currentVelocity + change * omega) * dt;

    currentVelocity = (currentVelocity - temp * omega) * exp;
    glm::vec3 result = target + (change + temp) * exp;
    // prevent overshoot
    if (glm::dot(originalTarget - current, result - originalTarget) > 0)
    {
        result = originalTarget;
        currentVelocity = (result - originalTarget) / dt;
    }

    return result;
}
} // namespace sky
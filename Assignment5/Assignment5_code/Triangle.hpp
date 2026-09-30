#pragma once

#include "Object.hpp"

#include <cstring>

// 辅助函数 用于判断浮点数是否为零
inline bool isZero(float value, float epsilon = 1e-6f)
{
    return std::fabs(value) < epsilon;
}

// TODO: Implement this function that tests whether the triangle
// that's specified bt v0, v1 and v2 intersects with the ray (whose
// origin is *orig* and direction is *dir*)
// Also don't forget to update tnear, u and v.

// 计算 E1 = v1 - v0。
// 计算 E2 = v2 - v0。
// 用 dir 和 E2 计算第一个叉积辅助向量。
// 计算行列式 det。
// 用 epsilon 排除平行或退化情况。
// 计算 1 / det。
// 计算 T = orig - v0。
// 求重心坐标 u。
// 检查 u 是否在合法范围。
// 计算第二个叉积辅助向量。
// 求重心坐标 v。
// 检查 v >= 0 和 u + v <= 1。
// 求射线参数 t。
// 排除 t 位于射线后方的情况。
// 更新 tnear、u、v。
// 返回 true。
// 任意检查失败时返回 false。
bool rayTriangleIntersect(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &orig,
                          const Vector3f &dir, float &tnear, float &u, float &v)
{
    Vector3f E1 = v1 - v0;
    Vector3f E2 = v2 - v0;
    float det = dotProduct(E1, crossProduct(dir, E2));
    if (isZero(det))
        return false;
    Vector3f S = orig - v0;
    float t_temp = (1 / det) * dotProduct(E2, crossProduct(S, E1));
    float u_temp = (1 / det) * dotProduct(S, crossProduct(dir, E2));
    float v_temp = (1 / det) * dotProduct(dir, crossProduct(S, E1));
    if (v_temp < 0 || u_temp < 0 || (u_temp + v_temp) > 1)
        return false;
    else if (t_temp < 0 || isZero(t_temp))
        return false;
    tnear = t_temp;
    u = u_temp;
    v = v_temp;
    return true;
}

class MeshTriangle : public Object
{
public:
    MeshTriangle(const Vector3f *verts, const uint32_t *vertsIndex, const uint32_t &numTris, const Vector2f *st)
    {
        uint32_t maxIndex = 0;
        for (uint32_t i = 0; i < numTris * 3; ++i)
            if (vertsIndex[i] > maxIndex)
                maxIndex = vertsIndex[i];
        maxIndex += 1;
        vertices = std::unique_ptr<Vector3f[]>(new Vector3f[maxIndex]);
        memcpy(vertices.get(), verts, sizeof(Vector3f) * maxIndex);
        vertexIndex = std::unique_ptr<uint32_t[]>(new uint32_t[numTris * 3]);
        memcpy(vertexIndex.get(), vertsIndex, sizeof(uint32_t) * numTris * 3);
        numTriangles = numTris;
        stCoordinates = std::unique_ptr<Vector2f[]>(new Vector2f[maxIndex]);
        memcpy(stCoordinates.get(), st, sizeof(Vector2f) * maxIndex);
    }

    bool intersect(const Vector3f &orig, const Vector3f &dir, float &tnear, uint32_t &index,
                   Vector2f &uv) const override
    {
        bool intersect = false;
        for (uint32_t k = 0; k < numTriangles; ++k)
        {
            const Vector3f &v0 = vertices[vertexIndex[k * 3]];
            const Vector3f &v1 = vertices[vertexIndex[k * 3 + 1]];
            const Vector3f &v2 = vertices[vertexIndex[k * 3 + 2]];
            float t, u, v;
            if (rayTriangleIntersect(v0, v1, v2, orig, dir, t, u, v) && t < tnear)
            {
                tnear = t;
                uv.x = u;
                uv.y = v;
                index = k;
                intersect |= true;
            }
        }

        return intersect;
    }

    void getSurfaceProperties(const Vector3f &, const Vector3f &, const uint32_t &index, const Vector2f &uv, Vector3f &N,
                              Vector2f &st) const override
    {
        const Vector3f &v0 = vertices[vertexIndex[index * 3]];
        const Vector3f &v1 = vertices[vertexIndex[index * 3 + 1]];
        const Vector3f &v2 = vertices[vertexIndex[index * 3 + 2]];
        Vector3f e0 = normalize(v1 - v0);
        Vector3f e1 = normalize(v2 - v1);
        N = normalize(crossProduct(e0, e1));
        const Vector2f &st0 = stCoordinates[vertexIndex[index * 3]];
        const Vector2f &st1 = stCoordinates[vertexIndex[index * 3 + 1]];
        const Vector2f &st2 = stCoordinates[vertexIndex[index * 3 + 2]];
        st = st0 * (1 - uv.x - uv.y) + st1 * uv.x + st2 * uv.y;
    }

    Vector3f evalDiffuseColor(const Vector2f &st) const override
    {
        float scale = 5;
        float pattern = (fmodf(st.x * scale, 1) > 0.5) ^ (fmodf(st.y * scale, 1) > 0.5);
        return lerp(Vector3f(0.815, 0.235, 0.031), Vector3f(0.937, 0.937, 0.231), pattern);
    }

    std::unique_ptr<Vector3f[]> vertices;
    uint32_t numTriangles;
    std::unique_ptr<uint32_t[]> vertexIndex;
    std::unique_ptr<Vector2f[]> stCoordinates;
};

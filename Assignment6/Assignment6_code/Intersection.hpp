//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_INTERSECTION_H
#define RAYTRACING_INTERSECTION_H
#include "Vector.hpp"
#include "Material.hpp"
class Object;
class Sphere;

struct Intersection
{
    Intersection()
    {
        // 是否命中
        happened = false;
        // 世界空间交点
        coords = Vector3f();
        // 焦点法线
        normal = Vector3f();
        // 沿光线到交点的参数或距离
        distance = std::numeric_limits<double>::max();
        // 被命中的对象
        obj = nullptr;
        // 对象材质
        m = nullptr;
    }
    bool happened;
    Vector3f coords;
    Vector3f normal;
    double distance;
    Object *obj;
    Material *m;
};
#endif // RAYTRACING_INTERSECTION_H

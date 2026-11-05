//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"

void Scene::buildBVH()
{
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum)
            {
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
    const Ray &ray,
    const std::vector<Object *> &objects,
    float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear)
        {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }

    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const

{
    // TO DO Implement Path Tracing Algorithm here

    Intersection hit = this->bvh->Intersect(ray);
    if (!hit.happened)
    {
        return Vector3f(0.0f);
    }
    if (hit.m->hasEmission())
    {
        return hit.m->m_emission;
    }

    Vector3f p = hit.coords;
    Vector3f n = hit.normal.normalized();
    Vector3f travelDir = Vector3f(ray.direction).normalized();
    Vector3f viewDir = -travelDir;
    Material *material = hit.m;

    // 计算直接光
    Vector3f l_dir(0.0f);

    // 修改版
    // 当前 Cornell Box 的起始参数；不是任意场景都通用的常量。
    const float rayOriginBias = 1e-4f;

    Intersection lightSample;
    float pdfArea = 0.0f;
    sampleLight(lightSample, pdfArea);

    Vector3f delta = lightSample.coords - p;
    float r2 = dotProduct(delta, delta);

    if (pdfArea > 0.0f && std::isfinite(pdfArea) &&
        r2 > rayOriginBias * rayOriginBias && std::isfinite(r2))
    {
        float r = std::sqrt(r2);
        Vector3f toLight = delta / r;
        Vector3f lightNormal = normalize(lightSample.normal);
        float cosSurface = std::max(0.0f, dotProduct(n, toLight));
        float cosLight = std::max(0.0f, dotProduct(lightNormal, -toLight));

        if (cosSurface > 0.0f && cosLight > 0.0f)
        {
            // 偏移后重新计算到同一灯点的方向和距离。
            Vector3f shadowOrigin = p + n * rayOriginBias;
            Vector3f shadowDelta = lightSample.coords - shadowOrigin;
            float shadowDistance = shadowDelta.norm();
            float endpointTolerance =
                std::max(1e-4f, 1e-6f * shadowDistance);

            if (std::isfinite(shadowDistance) &&
                shadowDistance > endpointTolerance)
            {
                Ray shadowRay(shadowOrigin, shadowDelta / shadowDistance);
                shadowRay.t_max =
                    static_cast<double>(shadowDistance) - endpointTolerance;

                // 区间内无交点才是无遮挡；不再要求区间内命中灯。
                if (!intersect(shadowRay).happened)
                {
                    Vector3f fr = material->eval(viewDir, toLight, n);
                    l_dir = lightSample.emit * fr * cosSurface * cosLight /
                            (r2 * pdfArea);
                }
            }
        }
    }

    // 原版本
    // Intersection lightSample;

    // // 对光源采样并获取PDF
    // float pdfArea = 0.0f;
    // sampleLight(lightSample, pdfArea);

    // Vector3f x = lightSample.coords;
    // Vector3f n_light = lightSample.normal.normalized();
    // Vector3f toLight = (x - p).normalized();
    // float r = 0.0f;
    // r = (x - p).norm();
    // // 计算BRDF
    // Vector3f f_r = material->eval(viewDir, x - p, n);

    // Ray light_ray(p, toLight);
    // Intersection shadow_inter = intersect(light_ray);
    // if (shadow_inter.distance - r > -EPSILON)
    // {
    //     // 没有被遮挡，套用换元后的直接光照公式
    //     l_dir = lightSample.emit *
    //             f_r *
    //             std::max(0.f, dotProduct(n, toLight)) *
    //             std::max(0.0f, dotProduct(n_light, -toLight)) /
    //             (r * r *
    //              pdfArea);
    // }

    // 计算间接光
    Vector3f l_indir(0.0f);

    // 俄罗斯转盘 小于等于 P_RR 则通过
    float survivalCount = get_random_float();
    if (survivalCount > RussianRoulette)
    {
        return l_dir;
    }

    // 通过后进行采样
    Vector3f bounceDir = normalize(material->sample(viewDir, n));
    // 获取当前PDF

    float pdfDirection = material->pdf(viewDir, bounceDir, n);
    float cosBounce = std::max(0.0f, dotProduct(n, bounceDir));

    if (!(pdfDirection > 0.0f) || !std::isfinite(pdfDirection) ||
        !(cosBounce > 0.0f))
    {
        return l_dir;
    }

    Ray nextRay(p + n * rayOriginBias, bounceDir);
    Intersection nextSample = intersect(nextRay);

    // 若采样到光源则跳过
    if (!nextSample.happened)
    {
        return l_dir;
    }
    if (nextSample.m->hasEmission())
    {
        return l_dir;
    }

    // 获取当前 f_r
    Vector3f f_r_q = material->eval(viewDir, bounceDir, n);

    l_indir = castRay(nextRay, depth + 1) *
              f_r_q *
              cosBounce /
              (pdfDirection *
               RussianRoulette);

    return l_dir + l_indir;
}

// Gemini参考版
// Vector3f Scene::castRay(const Ray &ray, int depth) const
// {
//     // 1. 求光线与场景交点
//     Intersection inter = intersect(ray);
//     if (!inter.happened)
//         return Vector3f(0.0f);

//     // 2. 如果直接打中光源，返回自发光 Le
//     if (inter.m->hasEmission())
//         return inter.m->getEmission();

//     Vector3f p = inter.coords;
//     Vector3f N = inter.normal.normalized();
//     Vector3f wo = -ray.direction; // 习惯将出射/入射方向统一为从 p 点向外指

//     // ================= 第一部分：直接光照 L_dir =================
//     Vector3f L_dir(0.0f);
//     Intersection light_inter;
//     float pdf_light = 0.0f;
//     sampleLight(light_inter, pdf_light); // 在光源上随机采样一点 x'

//     Vector3f x = light_inter.coords;
//     Vector3f ws = (x - p).normalized();            // p 指向光源 x' 的方向 (即 wi)
//     Vector3f NN = light_inter.normal.normalized(); // 光源表面法线 n'
//     float dist = (x - p).norm();

//     // 发射阴影测试光线，判断 p 和 x' 之间有没有物体遮挡 (注意加 EPSILON 防止自相交)
//     Ray light_ray(p, ws);
//     Intersection shadow_inter = intersect(light_ray);
//     if (shadow_inter.distance - dist > -1e-4)
//     {
//         // 没有被遮挡，套用换元后的直接光照公式
//         L_dir = light_inter.emit * inter.m->eval(ray.direction, ws, N) * dotProduct(ws, N) * dotProduct(-ws, NN) / (dist * dist * pdf_light);
//     }

//     // ================= 第二部分：间接光照 L_indir =================
//     Vector3f L_indir(0.0f);
//     // 俄罗斯轮盘赌测试
//     if (get_random_float() < RussianRoulette)
//     {
//         // 在半球上按材质属性采样一个新方向 wi
//         Vector3f wi = inter.m->sample(ray.direction, N).normalized();
//         Ray next_ray(p, wi);
//         Intersection next_inter = intersect(next_ray);

//         // 只有打中“非光源”物体才计算间接光（光源已经在 L_dir 算过了，避免重复累加）
//         if (next_inter.happened && !next_inter.m->hasEmission())
//         {
//             float pdf_hemi = inter.m->pdf(ray.direction, wi, N);
//             if (pdf_hemi > 1e-6)
//             {
//                 L_indir = castRay(next_ray, depth + 1) * inter.m->eval(ray.direction, wi, N) * dotProduct(wi, N) / (pdf_hemi * RussianRoulette);
//             }
//         }
//     }

//     return L_dir + L_indir;
// }
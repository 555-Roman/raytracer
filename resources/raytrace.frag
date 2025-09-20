#version 460 core

uniform uvec2 uResolution;
uniform vec3 cameraPosition;

in vec2 uv;
in vec3 rayDir;

out vec4 FragColor;



float RandomValue(inout uint rngState) {
    rngState = rngState * 747796405u + 2891336453u;
    uint result = ((rngState >> ((rngState >> 28u) + 4u)) ^ rngState) * 277803737u;
    result = (result >> 22u) ^ result;
    return result / 4294967296.0;
}
float RandomValueNormalDistribution(inout uint rngState) {
    float theta = 2 * 3.1415926 * RandomValue(rngState);
    float rho = sqrt(-2 * log(RandomValue(rngState)));
    return rho * cos(theta);
}
vec3 RandomDirection(inout uint rngState) {
    float x = RandomValueNormalDistribution(rngState);
    float y = RandomValueNormalDistribution(rngState);
    float z = RandomValueNormalDistribution(rngState);
    return normalize(vec3(x, y, z));
}
vec3 RandomHemisphereDirection(vec3 normal, inout uint rngState) {
    vec3 dir = RandomDirection(rngState);
    return dir * sign(dot(normal, dir));
}
vec2 RandomDirectionInCircle(inout uint rngState) {
    float theta = RandomValue(rngState) * 3.1415926 * 2;
    return vec2(cos(theta), sin(theta));
}


struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Material {
    vec3 color;
    vec3 emissionColor;
    float emissionStrength;
    float roughness;
    float transmission;
    float ior;
    float metalness;
};


struct Sphere {
    vec4 pos_radius;
    vec4 color_smoothness;
    vec4 emissionColor_emissionStrength;
    vec4 transmission_ior_metalness_tbd;
};
layout (std430, binding = 0) buffer SphereBuffer {
    Sphere spheres[];
};

struct Triangle {
    vec4 posA;
    vec4 posB;
    vec4 posC;
    vec4 normalA;
    vec4 normalB;
    vec4 normalC;
};
layout (std430, binding = 1) buffer TriangleBuffer {
    Triangle triangles[];
};

struct Model {
    uint triangleIndex;
    uint triangleCount;
    uint padding[2];

    vec4 boundMin;
    vec4 boundMax;

    vec4 color_smoothness;
    vec4 emissionColor_emissionStrength;

    vec4 translation;
    mat4 rotation;
    mat4 inverseRotation;
};
layout (std430, binding = 2) buffer ModelBuffer {
    Model models[];
};

struct HitInfo {
    bool didHit;
    float t;
    vec3 pos;
    vec3 normal;
    Material material;
    bool isBackFace;
};


vec3 debugColor = vec3(0.0);

HitInfo intersectRaySphere(Ray ray, Sphere sphere, bool detectBackFace) {
    HitInfo hitInfo;
    hitInfo.didHit = false;

    vec3 offsetRayOrigin = ray.origin - sphere.pos_radius.xyz;
    float a = dot(ray.dir, ray.dir);
    float b = 2 * dot(offsetRayOrigin, ray.dir);
    float c = dot(offsetRayOrigin, offsetRayOrigin) - sphere.pos_radius.w * sphere.pos_radius.w;
    float discriminant = b * b - 4 * a * c;

    if (discriminant > 0) {
        float t = (-b - sqrt(discriminant)) / (2 * a);
        if (detectBackFace) {
            float t1 = (-b + sqrt(discriminant)) / (2 * a);
            float t = min(max(t, 0), max(t1, 0));
        }

        if (t > 0) {
            hitInfo.didHit = true;
            hitInfo.t = t;
            hitInfo.pos = ray.origin + ray.dir * t;
            hitInfo.normal = normalize(hitInfo.pos - sphere.pos_radius.xyz);
            if (detectBackFace) hitInfo.isBackFace = dot(hitInfo.normal, ray.dir) > 0;
        }
    }
    return hitInfo;
}

HitInfo intersectRayTriangle(Ray ray, Triangle triangle) {
    vec3 edgeAB = triangle.posB.xyz - triangle.posA.xyz;
    vec3 edgeAC = triangle.posC.xyz - triangle.posA.xyz;
    vec3 normalVector =  cross(edgeAB, edgeAC);
    vec3 ao = ray.origin - triangle.posA.xyz;
    vec3 dao = cross(ao, ray.dir);

    float determinant = -dot(ray.dir, normalVector);
    float invDet = 1.0 / determinant;

    float t = dot(ao, normalVector) * invDet;
    float u = dot(edgeAC, dao) * invDet;
    float v = -dot(edgeAB, dao) * invDet;
    float w = 1.0 - u - v;

    HitInfo hitInfo;
    hitInfo.didHit = determinant >= 1e-6 && t > 0 && u >= 0 && v >= 0 && w >= 0;
    hitInfo.pos = ray.origin + ray.dir * t;
    hitInfo.normal = normalize(triangle.normalA.xyz * w + triangle.normalB.xyz * u + triangle.normalC.xyz * v);
    hitInfo.t = t;
    return hitInfo;
}

bool intersectRayBox(Ray ray, vec3 boxMin, vec3 boxMax) {
    vec3 invDir = 1 / ray.dir;
    vec3 tMin = (boxMin - ray.origin) * invDir;
    vec3 tMax = (boxMax - ray.origin) * invDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tNear <= tFar;
}

uniform vec3 modelMin;
uniform vec3 modelMax;
HitInfo calculateRayIntersection(Ray ray, bool detectBackFace) {
    HitInfo closestHit;
    closestHit.didHit = false;
    closestHit.t = 1.0 / 0.0;

    for (int i = 0; i < spheres.length(); i++) {
        Sphere sphere = spheres[i];
        HitInfo hitInfo = intersectRaySphere(ray, sphere, detectBackFace);
        if (hitInfo.didHit && hitInfo.t < closestHit.t) {
            closestHit = hitInfo;
            Material material;
            material.color = sphere.color_smoothness.rgb;
            material.emissionColor = sphere.emissionColor_emissionStrength.rgb;
            material.emissionStrength = sphere.emissionColor_emissionStrength.a;
            material.roughness = sphere.color_smoothness.a;
            material.transmission = sphere.transmission_ior_metalness_tbd.r;
            material.ior = sphere.transmission_ior_metalness_tbd.g;
            material.metalness = sphere.transmission_ior_metalness_tbd.b;
            closestHit.material = material;
        }
    }
    for (int modelIndex = 0; modelIndex < models.length(); modelIndex++) {
        bool didHitModel = false;
        Model model = models[modelIndex];
        Ray localRay;
        localRay.origin = ray.origin - model.translation.xyz;
        localRay.origin = mat3(model.rotation) * localRay.origin;
        localRay.dir = mat3(model.rotation) * ray.dir;
        if (!intersectRayBox(localRay, model.boundMin.xyz, model.boundMax.xyz)) continue;
        for (uint i = model.triangleIndex; i < model.triangleIndex+model.triangleCount; i++) {
            Triangle triangle = triangles[i];
            HitInfo hitInfo = intersectRayTriangle(localRay, triangle);
            if (hitInfo.didHit && hitInfo.t < closestHit.t) {
                didHitModel = true;
                closestHit = hitInfo;
                Material material;
                material.color = model.color_smoothness.rgb;
                material.emissionColor = model.emissionColor_emissionStrength.rgb;
                material.emissionStrength = model.emissionColor_emissionStrength.a;
                material.roughness = model.color_smoothness.a;
                material.transmission = 0.0;
                material.ior = 1.0;
                material.metalness = 0.0;
                closestHit.material = material;
            }
        }
        if (didHitModel) {
            closestHit.pos = mat3(model.inverseRotation) * closestHit.pos;
            closestHit.pos += model.translation.xyz;
    //        debugColor = normalize(closestHit.pos);
            closestHit.normal = normalize(mat3(model.inverseRotation) * closestHit.normal);
    //        debugColor = closestHit.normal;
        }
    }
    return closestHit;
}

float fresnelReflection(vec3 wi, vec3 normal, float iorI, float iorT) {
    float refractRatio = iorI / iorT;
    float cosAngleIn = -dot(wi, normal);
    float sinSqrAngleOfRefraction = refractRatio * refractRatio * (1 - cosAngleIn * cosAngleIn);
    if (sinSqrAngleOfRefraction >= 1) return 1; // Ray is fully reflected, no refraction occurs

    float cosAngleOfRefraction = sqrt(1 - sinSqrAngleOfRefraction);
    float denominatorPerpendicular = iorI * cosAngleIn + iorT * cosAngleOfRefraction;
    float denominatorParallel = iorI * cosAngleIn + iorT * cosAngleOfRefraction;

    if (min(denominatorPerpendicular, denominatorParallel) < 1E-8) return 1;

    // Perpendicular polarization
    float rPerpendicular = (iorI * cosAngleIn - iorT * cosAngleOfRefraction) / denominatorPerpendicular;
    rPerpendicular *= rPerpendicular;
    // Parallel polarization
    float rParallel = (iorT * cosAngleIn - iorI * cosAngleOfRefraction) / denominatorParallel;
    rParallel *= rParallel;

    // Return the average of the perpendicular and parallel polarizations
    return (rPerpendicular + rParallel) / 2;
}

void frisvad ( const vec3 n, out vec3 b1, out vec3 b2) {
    if(n.z < -0.9999999f) // Handle the singularity
    {
        b1 = vec3( 0.0f, -1.0f, 0.0f);
        b2 = vec3( -1.0f, 0.0f, 0.0f);
        return ;
    }
    const float a = 1.0f /(1.0f + n.z);
    const float b = -n.x*n.y*a;
    b1 = vec3(1.0f - n.x*n.x*a, b, -n.x);
    b2 = vec3(b, 1.0f - n.y*n.y*a, -n.y);
}

vec3 SampleVndf_Hemisphere(vec2 u, vec3 wi) {
    // sample a spherical cap in (-wi.z, 1]
    float phi = 2.0f * 3.1415926 * u.x;
    float z = fma((1.0f - u.y), (1.0f + wi.z), -wi.z);
    float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    vec3 c = vec3(x, y, z);
    // compute halfway direction;
    vec3 h = c + wi;
    // return without normalization (as this is done later)
    return h;
}

vec3 SampleVndf_GGX(vec2 u, vec3 wi, vec2 alpha) {
    // warp to the hemisphere configuration
    vec3 wiStd = normalize(vec3(wi.xy * alpha, wi.z));
    // sample the hemisphere (see implementation 2 or 3)
    vec3 wmStd = SampleVndf_Hemisphere(u, wiStd);
    // warp back to the ellipsoid configuration
    vec3 wm = normalize(vec3(wmStd.xy * alpha, wmStd.z));
    // return final normal
    return wm;
}

vec3 sampleGGXnormal(vec3 N, vec3 world_wi, vec2 u, vec2 alpha) {
    vec3 T, B;
    frisvad(N, T, B);
    vec3 local_wi = normalize(vec3(dot(world_wi, T), dot(world_wi, B), dot(world_wi, N)));
    vec3 local_m = SampleVndf_GGX(u, local_wi, alpha);
    vec3 world_m = normalize(local_m.x * T + local_m.y * B + local_m.z * N);
    return world_m;
}

vec3 GetEnvironmentLight(Ray ray) {
    return vec3(0.0);
    float a = 0.5*(ray.dir.y + 1.0);
    return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), a);
}

uniform int maxBounces_reflection;
uniform int maxBounces_transmission;
vec3 traceRay(Ray ray, inout uint rngState) {
    vec3 inLight = vec3(0.0);
    vec3 rayColor = vec3(1.0);
    uint reflectionBounces = 0;
    uint transmissionBounces = 0;
    bool isInsideMedium = false;
    while (reflectionBounces < maxBounces_reflection && transmissionBounces < maxBounces_transmission) {
        HitInfo hitInfo = calculateRayIntersection(ray, isInsideMedium);
        Material material = hitInfo.material;

        if (hitInfo.didHit) {
            vec3 microsurfaceNormal = sampleGGXnormal(hitInfo.normal, -ray.dir, vec2(RandomValue(rngState), RandomValue(rngState)), vec2(material.roughness));
            if (material.roughness < 0.01) microsurfaceNormal = hitInfo.normal;

            vec3 diffuseDir = normalize(hitInfo.normal + RandomDirection(rngState));
            vec3 specularReflectionDir = reflect(ray.dir, microsurfaceNormal);
            bool isDiffuseBounce = RandomValue(rngState) < material.metalness;
            vec3 reflectionDir = mix(specularReflectionDir, diffuseDir, isDiffuseBounce ? material.roughness : 1.0);
            vec3 specularTransmissionDir = refract(ray.dir, microsurfaceNormal, isInsideMedium ? material.ior : 1.0/material.ior);


            vec3 emittedLight = material.emissionColor * material.emissionStrength;
            inLight += emittedLight * rayColor;

            if (RandomValue(rngState) < material.metalness) {
                if (dot(specularReflectionDir, hitInfo.normal) < 0.0) break;
                ray.dir = specularReflectionDir;
                rayColor *= material.color;
                reflectionBounces++;
            } else {
                if (RandomValue(rngState) < fresnelReflection(ray.dir, microsurfaceNormal, isInsideMedium ? material.ior : 1.0, isInsideMedium ? 1.0 : material.ior)) {
                    if (dot(specularReflectionDir, hitInfo.normal) < 0.0) break;
                    ray.dir = specularReflectionDir;
                    rayColor *= vec3(1.0);
                    reflectionBounces++;
                } else {
                    if (RandomValue(rngState) < material.transmission) {
                        ray.dir = specularTransmissionDir;
                        transmissionBounces++;
                        isInsideMedium = !isInsideMedium;
                        rayColor *= material.color;
                    } else {
                        ray.dir = diffuseDir;
                        rayColor *= material.color;
                        reflectionBounces++;
                    }
                }
            }

            ray.origin = hitInfo.pos + 1e-5 * ray.dir;
        } else {
            inLight += GetEnvironmentLight(ray) * rayColor;
            break;
        }
    }
    return inLight;
}

uniform sampler2D uPrevFrame;
uniform uint renderedFrames;
uniform int samplesPerPixel;
uniform bool accumulate;
void main() {
    uvec2 pixelCoord = uvec2(uv * uResolution);
    uint rngState = pixelCoord.x * uResolution.x + pixelCoord.y + renderedFrames * 719393u;

    Ray ray = {cameraPosition, vec3(rayDir.xy + RandomDirectionInCircle(rngState)/uResolution.x, rayDir.z)};

    vec3 curr = vec3(0);
    for (int i = 0; i < samplesPerPixel; i++)
        curr += traceRay(ray, rngState);
    curr /= samplesPerPixel;

    if (debugColor != vec3(0.0))
        curr = debugColor;

    vec3 prev = texture(uPrevFrame, uv).rgb;
    float alpha = 1.0 / float(renderedFrames + 1u);
    vec3 blended = mix(prev, curr, alpha);
    FragColor = vec4(blended, 1.0);
}
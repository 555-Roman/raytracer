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
   return result / 4294967295.0;
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
   float theta = RandomValue(rngState) * 360;
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
   float smoothness;
};


struct Sphere {
   vec4 pos_radius;
   vec4 color_smoothness;
   vec4 emissionColor_emissionStrength;
};
layout (std430) buffer SphereBuffer {
   Sphere spheres[];
};

#define MAX_TRIANGLES 12
uniform int numTriangles;
uniform struct Triangle {
   vec3 posA;
   vec3 posB;
   vec3 posC;
   vec3 normalA;
   vec3 normalB;
   vec3 normalC;
   Material material;
} triangles[MAX_TRIANGLES];

struct HitInfo {
   bool didHit;
   float t;
   vec3 pos;
   vec3 normal;
   Material material;
};



HitInfo intersectRaySphere(Ray ray, Sphere sphere) {
   HitInfo hitInfo;
   hitInfo.didHit = false;

   vec3 offsetRayOrigin = ray.origin - sphere.pos_radius.xyz;
   float a = dot(ray.dir, ray.dir);
   float b = 2 * dot(offsetRayOrigin, ray.dir);
   float c = dot(offsetRayOrigin, offsetRayOrigin) - sphere.pos_radius.w * sphere.pos_radius.w;
   float discriminant = b * b - 4 * a * c;

   if (discriminant >= 0) {
      float t = (-b - sqrt(discriminant)) / (2 * a);

      if (t >= 0) {
         hitInfo.didHit = true;
         hitInfo.t = t;
         hitInfo.pos = ray.origin + ray.dir * t;
         hitInfo.normal = normalize(hitInfo.pos - sphere.pos_radius.xyz);
         Material material;
         material.color = sphere.color_smoothness.rgb;
         material.emissionColor = sphere.emissionColor_emissionStrength.rgb;
         material.emissionStrength = sphere.emissionColor_emissionStrength.a;
         material.smoothness = sphere.color_smoothness.a;
         hitInfo.material = material;
      }
   }
   return hitInfo;
}

HitInfo intersectRayTriangle(Ray ray, Triangle triangle) {
   vec3 edgeAB = triangle.posB - triangle.posA;
   vec3 edgeAC = triangle.posC - triangle.posA;
   vec3 normalVector =  cross(edgeAB, edgeAC);
   vec3 ao = ray.origin - triangle.posA;
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
   hitInfo.normal = normalize(triangle.normalA * w + triangle.normalB * u + triangle.normalC * v);
   hitInfo.t = t;
   hitInfo.material = triangle.material;
   return hitInfo;
}

HitInfo calculateRayIntersection(Ray ray) {
   HitInfo closestHit;
   closestHit.didHit = false;
   closestHit.t = 1.0 / 0.0;

   for (int i = 0; i < spheres.length(); i++) {
      Sphere sphere = spheres[i];
      HitInfo hitInfo = intersectRaySphere(ray, sphere);
      if (hitInfo.didHit && hitInfo.t < closestHit.t) {
         closestHit = hitInfo;
      }
   }
   for (int i = 0; i < numTriangles; i++) {
      Triangle triangle = triangles[i];
      HitInfo hitInfo = intersectRayTriangle(ray, triangle);
      if (hitInfo.didHit && hitInfo.t < closestHit.t) {
         closestHit = hitInfo;
      }
   }
   return closestHit;
}

vec3 GetEnvironmentLight(Ray ray) {
   float a = 0.5*(ray.dir.y + 1.0);
   return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), a);
}

uniform int maxBounces;
vec3 traceRay(Ray ray, inout uint rngState) {
   vec3 inLight = vec3(0.0);
   vec3 rayColor = vec3(1.0);
   for (int i = 0; i <= maxBounces; i++) {
      HitInfo hitInfo = calculateRayIntersection(ray);
      Material material = hitInfo.material;

      if (hitInfo.didHit) {
         ray.origin = hitInfo.pos;
         vec3 diffuseDir = normalize(hitInfo.normal + RandomDirection(rngState));
         vec3 specularDir = reflect(ray.dir, hitInfo.normal);
         ray.dir = mix(diffuseDir, specularDir, material.smoothness);

         vec3 emittedLight = material.emissionColor * material.emissionStrength;
         inLight += emittedLight * rayColor;
         rayColor *= material.color;
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
   if (accumulate) {
      vec3 prev = texture(uPrevFrame, uv).rgb;
      float alpha = 1.0 / float(renderedFrames + 1u);
      vec3 blended = mix(prev, curr, alpha);
      FragColor = vec4(blended, 1.0);
   } else
      FragColor = vec4(curr, 1.0);
}
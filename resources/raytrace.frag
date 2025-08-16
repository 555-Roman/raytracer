#version 330 core

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


struct Ray {
   vec3 origin;
   vec3 dir;
};

struct Material {
   vec3 color;
   vec3 emissionColor;
   float emissionStrength;
};

struct Sphere {
   vec3 pos;
   float radius;
   Material material;
};

const int numSpheres = 6;
Sphere spheres[numSpheres];

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

   vec3 offsetRayOrigin = ray.origin - sphere.pos;
   float a = dot(ray.dir, ray.dir);
   float b = 2 * dot(offsetRayOrigin, ray.dir);
   float c = dot(offsetRayOrigin, offsetRayOrigin) - sphere.radius * sphere.radius;
   float discriminant = b * b - 4 * a * c;

   if (discriminant >= 0) {
      float t = (-b - sqrt(discriminant)) / (2 * a);

      if (t >= 0) {
         hitInfo.didHit = true;
         hitInfo.t = t;
         hitInfo.pos = ray.origin + ray.dir * t;
         hitInfo.normal = normalize(hitInfo.pos - sphere.pos);
         hitInfo.material = sphere.material;
      }
   }
   return hitInfo;
}

HitInfo calculateRayIntersection(Ray ray) {
   HitInfo closestHit;
   closestHit.didHit = false;
   closestHit.t = 1.0 / 0.0;

   for (int i = 0; i < numSpheres; i++) {
      Sphere sphere = spheres[i];
      HitInfo hitInfo = intersectRaySphere(ray, sphere);
      if (hitInfo.didHit && hitInfo.t < closestHit.t) {
         closestHit = hitInfo;
      }
   }
   return closestHit;
}

uniform int maxBounces;
vec3 traceRay(Ray ray, inout uint rngState) {
   vec3 inLight = vec3(0.0);
   vec3 rayColor = vec3(1.0);
   for (int i = 0; i <= maxBounces; i++) {
      HitInfo hitInfo = calculateRayIntersection(ray);
      if (hitInfo.didHit) {
         ray.origin = hitInfo.pos;
         ray.dir = RandomHemisphereDirection(hitInfo.normal, rngState);

         Material material = hitInfo.material;
         vec3 emittedLight = material.emissionColor * material.emissionStrength;
         inLight += emittedLight * rayColor;
         rayColor *= material.color;
      } else {
         break;
      }
   }
   return inLight;
}

uniform sampler2D uPrevFrame;
uniform uint renderedFrames;
void main() {
   uvec2 pixelCoord = uvec2(uv * uResolution);
   uint rngState = pixelCoord.x * uResolution.x + pixelCoord.y + renderedFrames * 719393u;

   Ray ray = {cameraPosition, rayDir};
   Material sphereMat0 = {vec3(0.8, 0.0, 0.8), vec3(0.0), 0.0};
   Material sphereMat1 = {vec3(0.8, 0.0, 0.0), vec3(0.0), 0.0};
   Material sphereMat2 = {vec3(0.8, 0.8, 0.0), vec3(0.0), 0.0};
   Material sphereMat3 = {vec3(0.0, 0.8, 0.0), vec3(0.0), 0.0};
   Material sphereMat4 = {vec3(0.8, 0.8, 0.8), vec3(0.0), 0.0};
   Material sphereMat5 = {vec3(0.0, 0.0, 0.0), vec3(1.0), 5.0};
   Sphere sphere0 = {vec3(0.0, -100.0, 0.0), 100.0, sphereMat0};
   Sphere sphere1 = {vec3(0.0, 1.0, 0.0), 1.0, sphereMat1};
   Sphere sphere2 = {vec3(-2.0, 0.75, 0.0), 0.75, sphereMat2};
   Sphere sphere3 = {vec3(-3.5, 0.5, 0.0), 0.5, sphereMat3};
   Sphere sphere4 = {vec3(2.5, 1.25, 0.0), 1.25, sphereMat4};
   Sphere sphere5 = {vec3(-100.0, 50.0, 100.0), 100.0, sphereMat5};

   spheres[0] = sphere0;
   spheres[1] = sphere1;
   spheres[2] = sphere2;
   spheres[3] = sphere3;
   spheres[4] = sphere4;
   spheres[5] = sphere5;



   vec3 prev = texture(uPrevFrame, uv).rgb;
   vec3 curr = traceRay(ray, rngState);
   float alpha = 1.0 / float(renderedFrames + 1u);
   vec3 blended = mix(prev, curr, alpha);
   FragColor = vec4(blended, 1.0);
}
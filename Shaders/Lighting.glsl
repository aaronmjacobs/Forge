struct DirectionalLight
{
   vec4 color;
   vec4 direction;
};

struct PointLight
{
   vec4 colorRadius;
   vec4 position;
};

struct SpotLight
{
   vec4 colorRadius;
   vec4 positionBeamAngle;
   vec4 directionCutoffAngle;
};

struct LightingParams
{
   vec3 diffuseColor;
   vec3 specularColor;
   float shininess;

   vec3 surfacePosition;
   vec3 surfaceNormal;

   vec3 cameraPosition;
};

float calcAttenuation(vec3 toLight, float radius)
{
   float squaredDist = dot(toLight, toLight);
   float squaredFalloff = (1.0 / (1.0 + squaredDist));

   float dist = sqrt(squaredDist);
   float linearFalloff = 1.0 - clamp(dist / radius, 0.0, 1.0);

   return squaredFalloff * linearFalloff;
}

vec3 calcAmbient(vec3 lightColor, vec3 diffuseColor)
{
   return lightColor * diffuseColor * 0.1;
}

vec3 calcDiffuse(vec3 lightColor, vec3 diffuseColor, vec3 surfaceNormal, vec3 toLightDirection)
{
   float diffuseAmount = max(0.0, dot(surfaceNormal, toLightDirection));

   return lightColor * (diffuseColor * diffuseAmount) * 0.9;
}

vec3 calcSpecular(vec3 lightColor, vec3 specularColor, float shininess, vec3 surfacePosition, vec3 surfaceNormal, vec3 toLightDirection, vec3 cameraPosition)
{
   vec3 toCamera = normalize(cameraPosition - surfacePosition);
   vec3 reflection = reflect(-toLightDirection, surfaceNormal);

   float specularBase = max(0.0, dot(toCamera, reflection));
   float specularAmount = pow(specularBase, max(1.0, shininess));

   return lightColor * (specularColor * specularAmount);
}

vec3 calcDirectionalLighting(DirectionalLight directionalLight, LightingParams lightingParams)
{
   vec3 lightColor = directionalLight.color.rgb;
   vec3 lightDirection = directionalLight.direction.xyz;

   vec3 ambient = calcAmbient(lightColor, lightingParams.diffuseColor);
   vec3 diffuse = calcDiffuse(lightColor, lightingParams.diffuseColor, lightingParams.surfaceNormal, -lightDirection);
   vec3 specular = calcSpecular(lightColor, lightingParams.specularColor, lightingParams.shininess, lightingParams.surfacePosition, lightingParams.surfaceNormal, -lightDirection, lightingParams.cameraPosition);

   return ambient + (diffuse + specular);
}

vec3 calcPointLighting(PointLight pointLight, LightingParams lightingParams)
{
   vec3 lightColor = pointLight.colorRadius.rgb;
   vec3 lightPosition = pointLight.position.xyz;
   float lightRadius = pointLight.colorRadius.w;

   vec3 toLight = lightPosition - lightingParams.surfacePosition;
   vec3 toLightDirection = normalize(toLight);

   vec3 ambient = calcAmbient(lightColor, lightingParams.diffuseColor);
   vec3 diffuse = calcDiffuse(lightColor, lightingParams.diffuseColor, lightingParams.surfaceNormal, toLightDirection);
   vec3 specular = calcSpecular(lightColor, lightingParams.specularColor, lightingParams.shininess, lightingParams.surfacePosition, lightingParams.surfaceNormal, toLightDirection, lightingParams.cameraPosition);

   float attenuation = calcAttenuation(toLight, lightRadius);

   return (ambient + (diffuse + specular)) * attenuation;
}

vec3 calcSpotLighting(SpotLight spotLight, LightingParams lightingParams)
{
   vec3 lightColor = spotLight.colorRadius.rgb;
   vec3 lightPosition = spotLight.positionBeamAngle.xyz;
   vec3 lightDirection = spotLight.directionCutoffAngle.xyz;
   float lightRadius = spotLight.colorRadius.w;
   float lightBeamAngle = spotLight.positionBeamAngle.w;
   float lightCutoffAngle = spotLight.directionCutoffAngle.w;

   vec3 toLight = lightPosition - lightingParams.surfacePosition;
   vec3 toLightDirection = normalize(toLight);

   vec3 ambient = calcAmbient(lightColor, lightingParams.diffuseColor);
   vec3 diffuse = calcDiffuse(lightColor, lightingParams.diffuseColor, lightingParams.surfaceNormal, toLightDirection);
   vec3 specular = calcSpecular(lightColor, lightingParams.specularColor, lightingParams.shininess, lightingParams.surfacePosition, lightingParams.surfaceNormal, toLightDirection, lightingParams.cameraPosition);

   float attenuation = calcAttenuation(toLight, lightRadius);

   float spotAngle = acos(dot(lightDirection, -toLightDirection));
   float clampedAngle = clamp(spotAngle, lightBeamAngle, lightCutoffAngle);
   float spotMultiplier = 1.0 - ((clampedAngle - lightBeamAngle) / (lightCutoffAngle - lightBeamAngle));

   return (ambient + (diffuse + specular)) * attenuation * spotMultiplier;
}

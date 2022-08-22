#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const bool kOutputHDR = false;
layout(constant_id = 1) const bool kWithBloom = false;

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

// Below logic borrowed from the Xbox ATG samples (https://github.com/microsoft/Xbox-ATG-Samples/blob/master/Kits/ATGTK/HDR/HDRCommon.hlsli)

const float g_MaxNitsFor2084 = 10000.0f;

// Color rotation matrix to rotate Rec.709 color primaries into Rec.2020
const mat3 from709to2020 =
{
    { 0.6274040f, 0.3292820f, 0.0433136f },
    { 0.0690970f, 0.9195400f, 0.0113612f },
    { 0.0163916f, 0.0880132f, 0.8955950f }
};

// Rotation matrix describing a custom color space which is bigger than Rec.709, but a little smaller than P3
// This enhances colors, especially in the SDR range, by being a little more saturated. This can be used instead
// of from709to2020.
const mat3 fromExpanded709to2020 =
{
    { 0.6274040f, 0.3292820f, 0.0433136f },
    { 0.0457456, 0.941777, 0.0124772 },
    { -0.00121055, 0.0176041, 0.983607 }
};

vec3 NormalizeHDRSceneValue(vec3 hdrSceneValue, float paperWhiteNits)
{
    vec3 normalizedLinearValue = hdrSceneValue * paperWhiteNits / g_MaxNitsFor2084;
    return normalizedLinearValue;       // Don't clamp between [0..1], so we can still perform operations on scene values higher than 10,000 nits
}

vec3 LinearToST2084(vec3 normalizedLinearValue)
{
    vec3 ST2084 = pow((0.8359375f + 18.8515625f * pow(abs(normalizedLinearValue), vec3(0.1593017578f))) / (1.0f + 18.6875f * pow(abs(normalizedLinearValue), vec3(0.1593017578f))), vec3(78.84375f));
    return ST2084;  // Don't clamp between [0..1], so we can still perform operations on scene values higher than 10,000 nits
}

vec4 ConvertToHDR10(vec4 hdrSceneValue, float paperWhiteNits)
{
    vec3 rec2020 = /*from709to2020 * */hdrSceneValue.rgb;                             // Rotate Rec.709 color primaries into Rec.2020 color primaries
    vec3 normalizedLinearValue = NormalizeHDRSceneValue(rec2020, paperWhiteNits);     // Normalize using paper white nits to prepare for ST.2084
    vec3 HDR10 = LinearToST2084(normalizedLinearValue);                               // Apply ST.2084 curve

    return vec4(HDR10.rgb, hdrSceneValue.a);
}

vec3 tonemap(vec3 hdrColor)
{
   const float curve = 8.0;

   if (kOutputHDR)
   {
      return ConvertToHDR10(vec4(hdrColor, 1.0), 100.0).rgb; // TODO paperwhite
   }

   vec3 clampedHdrColor = max(vec3(0.0), hdrColor);
   vec3 sdrColor = clampedHdrColor - (pow(pow(clampedHdrColor, vec3(curve)) + 1.0, vec3(1.0 / curve)) - 1.0);

   return sdrColor;
}

void main()
{
   vec3 hdrColor = texture(hdrTexture, inTexCoord).rgb;
   if (kWithBloom)
   {
      vec3 bloom = texture(bloomTexture, inTexCoord).rgb;
      hdrColor = mix(hdrColor, bloom, 0.05);
   }

   outColor = vec4(tonemap(hdrColor), 1.0);
}

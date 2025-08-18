#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable

const int kTonemappingAlgorithm_None = 0;
const int kTonemappingAlgorithm_Curve = 1;
const int kTonemappingAlgorithm_Reinhard = 2;
const int kTonemappingAlgorithm_TonyMcMapface = 3;
const int kTonemappingAlgorithm_DoubleFine = 4;

layout(constant_id = 0) const int kTonemappingAlgorithm = kTonemappingAlgorithm_None;
layout(constant_id = 1) const bool kOutputHDR = false;
layout(constant_id = 2) const bool kWithBloom = false;
layout(constant_id = 3) const bool kWithUI = false;
layout(constant_id = 4) const bool kShowTestPattern = false;

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;
layout(set = 0, binding = 2) uniform sampler2D uiTexture;
layout(set = 0, binding = 3) uniform sampler3D lutTexture;

layout(std430, set = 0, binding = 4) uniform TonemapData
{
   float bloomStrength;
   float peakBrightness;

   vec2 ToeAndInv;
   float Shoulder;
   float Hotspot;
   float HuePreservation;
} params;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

const float kPaperwhiteNits = 100.0;

////////////////////////////////////////////////////////////////////////////////
// Xbox ATG code begin
//
// Logic borrowed from https://github.com/microsoft/Xbox-ATG-Samples/blob/master/Kits/ATGTK/HDR/HDRCommon.hlsli
////////////////////////////////////////////////////////////////////////////////

const float g_MaxNitsFor2084 = 10000.0f;

// Color rotation matrix to rotate Rec.709 color primaries into Rec.2020
const mat3 from709to2020 =
{
    { 0.6274040f, 0.0690970f, 0.0163916f },
    { 0.3292820f, 0.9195400f, 0.0880132f },
    { 0.0433136f, 0.0113612f, 0.8955950f }
};

// Rotation matrix describing a custom color space which is bigger than Rec.709, but a little smaller than P3
// This enhances colors, especially in the SDR range, by being a little more saturated. This can be used instead
// of from709to2020.
const mat3 fromExpanded709to2020 =
{
    { 0.6274040f, 0.0457456, 0.00121055 },
    { 0.3292820f, 0.941777, 0.0176041 },
    { -0.0433136f, 0.0124772, 0.983607 }
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
    vec3 rec2020 = from709to2020 * hdrSceneValue.rgb;                                 // Rotate Rec.709 color primaries into Rec.2020 color primaries
    vec3 normalizedLinearValue = NormalizeHDRSceneValue(rec2020, paperWhiteNits);     // Normalize using paper white nits to prepare for ST.2084
    vec3 HDR10 = LinearToST2084(normalizedLinearValue);                               // Apply ST.2084 curve

    return vec4(HDR10.rgb, hdrSceneValue.a);
}

////////////////////////////////////////////////////////////////////////////////
// Xbox ATG code end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Tony McMapface code begin
//
// Copyright (c) 2023 Tomasz Stachowiak
////////////////////////////////////////////////////////////////////////////////

vec3 tony_mc_mapface(vec3 stimulus)
{
    // Apply a non-linear transform that the LUT is encoded with.
    const vec3 encoded = stimulus / (stimulus + 1.0);

    // Align the encoded range to texel centers.
    const float LUT_DIMS = 48.0;
    const vec3 uv = encoded * ((LUT_DIMS - 1.0) / LUT_DIMS) + 0.5 / LUT_DIMS;

    // Note: for OpenGL, do `uv.y = 1.0 - uv.y`

    return texture(lutTexture, uv).rgb;
}

////////////////////////////////////////////////////////////////////////////////
// Tony McMapface code end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Double Fine code begin
//
// HSV --> RGB borrowed from https://www.chilliant.com/rgb2hsv.html
////////////////////////////////////////////////////////////////////////////////

float ApplyToe(float Value, float InvToe)
{
   float Bias = 3; // Only apply toe to the lower third of the range at most.
   float Term = clamp(1 - Bias * Value * InvToe, 0, 1);
   return Value * (1 - Term * Term * Term);
}

float ApplyShoulder(float Value, float Scale, float White)
{
   if (Value < Scale * White)
      return Value;

   float A = (1 - Scale) * White;
   float B = Value + A - White;
   float C = B * B + B * A + A * A;
   return White - A * A * A / max(C, 1e-7);

   //return White - exp((White - Value) / (A + 1e-7) - 1) * A; // Old exp method.
}

float GetLuminance(vec3 Color)
{
   return dot(Color, vec3(0.28, 0.58, 0.14));
}

vec3 ApplyHue(vec3 Color, vec3 Hue, float Amount, float LuminancePreservation)
{
   float Max = max(max(Color.r, Color.g), Color.b);
   float Min = min(min(Color.r, Color.g), Color.b);
   float Saturation = Max - Min;
   vec3 Result = mix(Color, Min + Hue * Saturation, Amount);

   float LuminanceLost = max(GetLuminance(Color) - GetLuminance(Result), 0);
   LuminanceLost = ApplyToe(LuminanceLost, 1);
   Result += LuminanceLost * LuminancePreservation;

   return Result;
}

vec3 ApplyLuminanceSDR(vec3 Color, float Luminance, float White, float Amount)
{
   float SourceLum = GetLuminance(Color);
   float TargetLum = ApplyShoulder(Luminance, params.Shoulder, White);
   float Correction = max(TargetLum - SourceLum, 0) / max(White - SourceLum, 1e-7);
   Correction = ApplyToe(Correction, 0.05);

   return mix(Color, vec3(White), Correction * Amount);
}

vec3 ApplyLuminanceHDR(vec3 Tonemapped, vec3 Original, float Amount)
{
   vec3 Correction = max(Original - Tonemapped, 0);
   float Luminance = GetLuminance(Correction);
   Correction = 0.5 * Correction + 0.25 * Luminance + 0.25 * ApplyToe(Luminance, 0.05);

   return Tonemapped + Correction * clamp(2 * Amount, 0, 1);
}

vec3 DFTonemap(vec3 Color, bool bIsHDR)
{
    float White = 1;

    // Calculate the normalized hue.
    float Max = max(max(Color.r, Color.g), Color.b);
    float Min = min(min(Color.r, Color.g), Color.b);
    float Saturation = Max - Min;
    vec3 Hue = (Color - Min) / max(Saturation, 1e-7);

    // Apply toe.
    Max = ApplyToe(Max, params.ToeAndInv.y);
    Min = ApplyToe(Min, params.ToeAndInv.y);
    Saturation = Max - Min;
    Color = Min + Hue * Saturation;

    // Add hotspot by limiting overbright saturation.
    float Luminance = GetLuminance(Color);
    float SaturationLimit = White + (1 - params.Hotspot * params.Hotspot) * (Max - White);
    Saturation = ApplyShoulder(Saturation, 1 - 0.5 * params.Hotspot * params.Hotspot, SaturationLimit);
    vec3 Tonemapped = Luminance + (Hue - GetLuminance(Hue)) * Saturation;

    // Apply shoulder.
    Tonemapped.r = ApplyShoulder(Tonemapped.r, params.Shoulder, White);
    Tonemapped.g = ApplyShoulder(Tonemapped.g, params.Shoulder, White);
    Tonemapped.b = ApplyShoulder(Tonemapped.b, params.Shoulder, White);

    // Preserve hue shifted by shoulder.
    float LuminancePreservation = clamp(2 * params.Hotspot, 0, 1);
    Tonemapped = ApplyHue(Tonemapped, Hue, params.HuePreservation, LuminancePreservation);

    // Preserve luminance lost by shoulder.
    Tonemapped = ApplyLuminanceSDR(Tonemapped, Luminance, White, LuminancePreservation);

    if (bIsHDR)
        Tonemapped = ApplyLuminanceHDR(Tonemapped, Color, LuminancePreservation);

    return Tonemapped;
}

vec3 HUEtoRGB(in float H)
{
   float R = abs(H * 6 - 3) - 1;
   float G = 2 - abs(H * 6 - 2);
   float B = 2 - abs(H * 6 - 4);
   return clamp(vec3(R, G, B), vec3(0), vec3(1));
}

vec3 HSVtoRGB(in vec3 HSV)
{
   vec3 RGB = HUEtoRGB(HSV.x);
   return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

vec3 TestPattern(vec2 UV, float Power, float Bands)
{
  float Hue = floor(Bands * (1-UV.y)) / Bands;
  float Value = pow(2 * UV.x, Power);
  vec3 HSV = vec3(Hue, 1, Value);
  return HSVtoRGB(HSV);
}

////////////////////////////////////////////////////////////////////////////////
// Double Fine code end
////////////////////////////////////////////////////////////////////////////////

vec3 tonemap(vec3 hdrColor)
{
   const float kBrightnessScale = kOutputHDR ? (params.peakBrightness / kPaperwhiteNits) : 1.0f;

   vec3 scaledHdrColor = max(vec3(0.0), hdrColor / kBrightnessScale);
   vec3 tonemappedColor = scaledHdrColor;

   if (kTonemappingAlgorithm == kTonemappingAlgorithm_Curve)
   {
      const float curve = 4.0;
      tonemappedColor = scaledHdrColor - (pow(pow(scaledHdrColor, vec3(curve)) + 1.0, vec3(1.0 / curve)) - 1.0);
   }
   else if (kTonemappingAlgorithm == kTonemappingAlgorithm_Reinhard)
   {
      tonemappedColor = scaledHdrColor / (scaledHdrColor + vec3(1.0));
   }
   else if (kTonemappingAlgorithm == kTonemappingAlgorithm_TonyMcMapface)
   {
      tonemappedColor = tony_mc_mapface(scaledHdrColor);
   }
   else if (kTonemappingAlgorithm == kTonemappingAlgorithm_DoubleFine)
   {
      tonemappedColor = DFTonemap(scaledHdrColor, kOutputHDR);
   }

   tonemappedColor = clamp(tonemappedColor, 0.0, 1.0) * kBrightnessScale;

   return tonemappedColor;
}

vec3 srgbToLinear(vec3 sRGB)
{
   vec3 a = sRGB / 12.92;
   vec3 b = pow((sRGB + 0.055) / 1.055, vec3(2.4));
   vec3 c = step(vec3(0.04045), sRGB);
   return mix(a, b, c);
}

void main()
{
   vec3 hdrColor;
   if (kShowTestPattern)
   {
      hdrColor = TestPattern(inTexCoord, 4, 24);
   }
   else
   {
      hdrColor = texture(hdrTexture, inTexCoord).rgb;

      if (kWithBloom)
      {
         vec3 bloom = texture(bloomTexture, inTexCoord).rgb;
         hdrColor = mix(hdrColor, bloom, params.bloomStrength);
      }
   }

   vec3 tonemappedColor = tonemap(hdrColor);

   if (kWithUI)
   {
      vec4 uiSrgb = texture(uiTexture, inTexCoord);
      vec3 uiLinear = srgbToLinear(uiSrgb.rgb);

      tonemappedColor = mix(tonemappedColor, uiLinear, uiSrgb.a);
   }

   if (kOutputHDR)
   {
      tonemappedColor = ConvertToHDR10(vec4(tonemappedColor, 1.0), kPaperwhiteNits).rgb;
   }
   else
   {
      tonemappedColor = clamp(tonemappedColor, 0.0, 1.0);
   }

   outColor = vec4(tonemappedColor, 1.0);
}

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

   float Toe;
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

float ApplyToe(float Value, float Toe)
{
   return Value * Value / (Value + 0.25 * Toe / (25 * Value + 1) + 1e-7);
}

float ApplyExpShoulder(float Value, float Scale, float White)
{
   if (Value < Scale * White)
   {
      return Value;
   }

   float Term = (1 - Scale) * White;
   return White - exp((White - Value) / (Term + 1e-7) - 1) * Term;
}

float GetLuminance(vec3 Color)
{
   return dot(Color, vec3(0.28, 0.58, 0.14));
}

vec3 ApplyHue(vec3 Color, vec3 Hue, float Amount)
{
   float Max = max(max(Color.r, Color.g), Color.b);
   float Min = min(min(Color.r, Color.g), Color.b);
   return mix(Color, Min + Hue * (Max - Min), Amount);
}

vec3 ApplyLuminanceSDR(vec3 Color, float Luminance, float White, float Amount, float Soften)
{
   float SourceLum = GetLuminance(Color);
   float Correction = max(Luminance - SourceLum, 0) / (max(White - SourceLum, 0) + 1e-7);
   float EdgeSoften = Soften * White;
   Correction *= Correction / (Correction + EdgeSoften);
   return mix(Color, vec3(White), Correction * Amount);
}

vec3 ApplyLuminanceHDR(vec3 Color, vec3 Original, float Amount)
{
   float Correction = GetLuminance(Original) - GetLuminance(Color);
   float EdgeSoften = 4 * GetLuminance(Color);
   Correction *= Correction / (Correction + EdgeSoften + 1e-7);
   return mix(Original, Color + Correction, Amount);
}

vec3 DFTonemap(vec3 Color, bool bIsHDR, float HdrWhite)
{
   float White = bIsHDR ? HdrWhite : 1;

   // Calculate the normalized hue.
   float Max = max(max(Color.r, Color.g), Color.b);
   float Min = min(min(Color.r, Color.g), Color.b);
   vec3 Hue = (Color - Min) / (Max - Min + 1e-7);

   // Increase contrast to keep mid gray levels with toe.
   Color *= 0.333 / ApplyToe(0.333, params.Toe);

   // Apply toe.
   Color.r = ApplyToe(Color.r, params.Toe);
   Color.g = ApplyToe(Color.g, params.Toe);
   Color.b = ApplyToe(Color.b, params.Toe);

   // Add hotspot by limiting overbright saturation.
   float Lum = GetLuminance(Color);
   float Saturation = ApplyExpShoulder(Max - Min, 1 - 0.5 * params.Hotspot, White);
   vec3 Tonemapped = Lum + (Hue - GetLuminance(Hue)) * Saturation;
   Tonemapped = mix(Color, Tonemapped, params.Hotspot * params.Hotspot);

   // Apply shoulder.
   Tonemapped.r = ApplyExpShoulder(Tonemapped.r, params.Shoulder, White);
   Tonemapped.g = ApplyExpShoulder(Tonemapped.g, params.Shoulder, White);
   Tonemapped.b = ApplyExpShoulder(Tonemapped.b, params.Shoulder, White);
   float TonemappedLum = GetLuminance(Tonemapped);

   // Preserve original hue.
   Tonemapped = ApplyHue(Tonemapped, Hue, params.HuePreservation);
   float HuePreservedLum = GetLuminance(Tonemapped);

   // Preserve luminance lost by hue preservation.
   float LuminancePreservation = clamp(2 * params.Hotspot, 0.0, 1.0);
   Tonemapped = ApplyLuminanceSDR(Tonemapped, TonemappedLum, White, LuminancePreservation, 0.1);

   // Preserve luminance lost by tonemapping.
   if (bIsHDR)
   {
      Tonemapped = ApplyLuminanceHDR(Tonemapped, Color, LuminancePreservation);
   }
   else
   {
      Tonemapped = ApplyLuminanceSDR(Tonemapped, ApplyExpShoulder(GetLuminance(Color), params.Shoulder, White), White, LuminancePreservation, 4);
   }

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

   vec3 clampedHdrColor = max(vec3(0.0), hdrColor / kBrightnessScale);
   vec3 tonemappedColor = clampedHdrColor;

   if (kTonemappingAlgorithm == kTonemappingAlgorithm_Curve)
   {
      const float curve = 4.0;
      tonemappedColor = clampedHdrColor - (pow(pow(clampedHdrColor, vec3(curve)) + 1.0, vec3(1.0 / curve)) - 1.0);
   }
   else if (kTonemappingAlgorithm == kTonemappingAlgorithm_Reinhard)
   {
      tonemappedColor = clampedHdrColor / (clampedHdrColor + vec3(1.0));
   }
   else if (kTonemappingAlgorithm == kTonemappingAlgorithm_TonyMcMapface)
   {
      tonemappedColor = tony_mc_mapface(clampedHdrColor);
   }
   else if (kTonemappingAlgorithm == kTonemappingAlgorithm_DoubleFine)
   {
      tonemappedColor = DFTonemap(clampedHdrColor, kOutputHDR, 1.0);
   }

   tonemappedColor *= kBrightnessScale;

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

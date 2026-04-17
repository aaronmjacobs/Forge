#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable

const int kTonemappingAlgorithm_None = 0;
const int kTonemappingAlgorithm_Curve = 1;
const int kTonemappingAlgorithm_Reinhard = 2;
const int kTonemappingAlgorithm_TonyMcMapface = 3;
const int kTonemappingAlgorithm_DoubleFine = 4;

const int kColorGamut_Rec709 = 0;
const int kColorGamut_Rec2020 = 1;
const int kColorGamut_P3 = 2;

const int kTransferFunction_Linear = 0;
const int kTransferFunction_sRGB = 1;
const int kTransferFunction_PQ = 2;
const int kTransferFunction_HLG = 3;

layout(constant_id = 0) const int kTonemappingAlgorithm = kTonemappingAlgorithm_None;
layout(constant_id = 1) const int kColorGamut = kColorGamut_Rec709;
layout(constant_id = 2) const int kTransferFunction = kTransferFunction_Linear;
layout(constant_id = 3) const bool kOutputHDR = false;
layout(constant_id = 4) const bool kWithBloom = false;
layout(constant_id = 5) const bool kWithUI = false;
layout(constant_id = 6) const bool kShowTestPattern = false;

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;
layout(set = 0, binding = 2) uniform sampler2D uiTexture;
layout(set = 0, binding = 3) uniform sampler3D lutTexture;

layout(std430, set = 0, binding = 4) uniform TonemapData
{
   float bloomStrength;

   float paperWhiteNits;
   float peakBrightnessNits;

   float Toe;
   float Shoulder;
   float Hotspot;
   float HuePreservation;
} params;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

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

vec3 linearToSRGB(vec3 linear)
{
   return mix(1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055, linear * 12.92, lessThanEqual(linear, vec3(0.0031308)));
}

vec3 srgbToLinear(vec3 sRGB)
{
   return mix(sRGB / 12.92, pow((sRGB + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), sRGB));
}

vec3 linearToPQ(vec3 linear)
{
   return pow((0.8359375 + 18.8515625 * pow(abs(linear), vec3(0.1593017578125))) / (1.0 + 18.6875 * pow(abs(linear), vec3(0.1593017578125))), vec3(78.84375));
}

vec3 linearToHLG(vec3 linear)
{
   return mix(0.17883277 * log(12.0 * linear - 0.28466892) + 0.55991073, sqrt(3.0 * linear), lessThanEqual(linear, vec3(1.0 / 12.0)));
}

vec3 normalizeHDR(vec3 linearValue, float reference, float peak)
{
   return linearValue * reference / peak;
}

vec3 tonemap(vec3 hdrColor)
{
   const float kBrightnessScale = kOutputHDR ? (params.peakBrightnessNits / params.paperWhiteNits) : 1.0f;

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
      tonemappedColor = DFTonemap(scaledHdrColor, kOutputHDR, 1.0);
   }

   tonemappedColor = clamp(tonemappedColor, 0.0, 1.0) * kBrightnessScale;

   return tonemappedColor;
}

vec3 convertGamut(vec3 color)
{
   // Color space rotation matrices generated from https://www.colour-science.org:8010/apps/rgb_colourspace_transformation_matrix

   const mat3 k709to2020 =
   {
      { 0.6274038959, 0.0690972894, 0.0163914389 },
      { 0.3292830384, 0.9195403951, 0.0880133079 },
      { 0.0433130657, 0.0113623156, 0.8955952532 }
   };

   const mat3 k709toP3 =
   {
      { 0.8224619687, 0.0331941989, 0.0170826307 },
      { 0.1775380313, 0.9668058011, 0.0723974407 },
      { 0.0000000000, 0.0000000000, 0.9105199286 }
   };

   if (kColorGamut == kColorGamut_Rec2020)
   {
      return k709to2020 * color;
   }

   if (kColorGamut == kColorGamut_P3)
   {
      return k709toP3 * color;
   }

   return color;
}

vec3 applyTransferFunction(vec3 color)
{
   if (kTransferFunction == kTransferFunction_sRGB)
   {
      return linearToSRGB(color);
   }

   if (kTransferFunction == kTransferFunction_PQ)
   {
      return linearToPQ(normalizeHDR(color, params.paperWhiteNits, 10000.0));
   }

   if (kTransferFunction == kTransferFunction_HLG)
   {
      return linearToHLG(normalizeHDR(color, 0.5, 0.5 * 12.0));
   }

   return color;
}

vec3 encodeColor(vec3 color)
{
   return applyTransferFunction(convertGamut(color));
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

   outColor = vec4(encodeColor(tonemappedColor), 1.0);
}

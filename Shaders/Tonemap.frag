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

   vec2 ToeAndInv;
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

float ApplyToe(float Value, float InvToe)
{
   // Slope is 0 at the origin.
   // Slope is 1 at inflection point where Value == Toe.
   float Bias = 3; // Only apply toe to the lower third of the range at most.
   float Term = clamp(1 - Bias * Value * InvToe, 0.0, 1.0);
   return Value * (1 - Term * Term * Term);
}

float ApplyShoulder(float Value, float Scale, float White)
{
   if (Value < Scale * White)
      return Value;

   // Output approaches White as Value goes to infinity.
   // Slope is 1 at inflection point where Value == Scale * White.
   float A = (1 - Scale) * White;
   float B = Value + A - White;
   float C = B * B + B * A + A * A;
   return White - A * A * A / max(C, 1e-7);
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

   // Preserving darker hues can lose luminance, so add it back evenly.
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
   // Restore mix of RGB and Luminance lost by tonemapping.
   vec3 Correction = max(Original - Tonemapped, 0);
   float Luminance = GetLuminance(Correction);
   Correction = 0.5 * Correction + 0.25 * Luminance + 0.25 * ApplyToe(Luminance, 0.05);

   return Tonemapped + Correction * clamp(2 * Amount, 0.0, 1.0);
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
   float LuminancePreservation = clamp(2 * params.Hotspot, 0.0, 1.0);
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

vec3 linearToSRGB(vec3 linear)
{
   vec3 low = linear * 12.92;
   vec3 high = 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055;
   bvec3 chooseHigh = greaterThan(linear, vec3(0.0031308));

   return mix(low, high, chooseHigh);
}

vec3 srgbToLinear(vec3 sRGB)
{
   vec3 low = sRGB / 12.92;
   vec3 high = pow((sRGB + 0.055) / 1.055, vec3(2.4));
   bvec3 chooseHigh = greaterThan(sRGB, vec3(0.04045));

   return mix(low, high, chooseHigh);
}

vec3 linearToPQ(vec3 linear)
{
   const float c1 = 0.8359375;
   const float c2 = 18.8515625;
   const float c3 = 18.6875;

   const float m1 = 0.1593017578125;
   const float m2 = 78.84375;

   vec3 yM1 = pow(linear, vec3(m1));
   vec3 numerator = c1 + c2 * yM1;
   vec3 denominator = 1.0 + c3 * yM1;

   return pow(numerator / denominator, vec3(m2));
}

vec3 linearToHLG(vec3 linear)
{
   const float a = 0.17883277;
   const float b = 0.28466892;
   const float c = 0.55991073;

   vec3 low = sqrt(3.0 * linear);
   vec3 high = a * log(12.0 * linear - b) + c;
   bvec3 chooseHigh = greaterThan(linear, vec3(1.0 / 12.0));

   return mix(low, high, chooseHigh);
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
      tonemappedColor = DFTonemap(scaledHdrColor, kOutputHDR);
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

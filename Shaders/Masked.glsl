#if !defined(MASKED_GLSL)
#define MASKED_GLSL

bool passesMaskThreshold(float alpha)
{
   return alpha >= 0.5;
}

#endif

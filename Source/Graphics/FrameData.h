#pragma once

#include "Graphics/GraphicsContext.h"

#include <array>

template<typename T>
class FrameData
{
public:
   FrameData(const T& initialValue)
   {
      setAll(initialValue);
   }

   const T& get(const GraphicsContext& context) const
   {
      return data[context.getFrameIndex()];
   }

   void set(const GraphicsContext& context, const T& value)
   {
      data[context.getFrameIndex()] = value;
   }

   void setAll(const T& value)
   {
      data.fill(value);
   }

private:
   std::array<T, GraphicsContext::kMaxFramesInFlight> data;
};

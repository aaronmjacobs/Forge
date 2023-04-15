#pragma once

#include "Core/Memory/FrameAllocator.h"

#include <vector>

template<typename T>
using FrameVector = std::vector<T, FrameAllocator<T>>;

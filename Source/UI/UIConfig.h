#pragma once

// Defined as IMGUI_USER_CONFIG in Libraries.cmake

#include "Core/Assert.h"

//---- Define assertion handler. Defaults to calling DM_ASSERT().
#define IM_ASSERT(_EXPR) ASSERT(_EXPR)

//---- Don't define obsolete functions/enums names. Consider enabling from time to time after updating to avoid using soon-to-be obsolete function/names.
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

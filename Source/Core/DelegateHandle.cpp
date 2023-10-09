#include "Core/DelegateHandle.h"

DelegateHandle DelegateHandle::create()
{
   DelegateHandle handle;
   handle.id = ++counter;
   return handle;
}

// static
uint64_t DelegateHandle::counter = 0;

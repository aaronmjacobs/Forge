#pragma once

#if defined(__has_include)
#  if __has_include(<bit>)
#     include <bit>
#  endif
#endif

#if !defined(__cpp_lib_bit_cast)
#  include <cstring>
#  include <type_traits>
#endif

namespace Types
{
#if defined(__cpp_lib_bit_cast)
   using std::bit_cast;
#else
   template<typename To, typename From>
   To bit_cast(const From& from) noexcept requires (sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>)
   {
      To to;
      std::memcpy(&to, &from, sizeof(To));
      return to;
   }
#endif
}

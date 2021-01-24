#pragma once

#include "Core/Hash.h"

#include <GLFW/glfw3.h>

enum class Key
{
   // Unknown key
   Unknown = GLFW_KEY_UNKNOWN,

   // Printable keys
   Space = GLFW_KEY_SPACE,
   Apostrophe = GLFW_KEY_APOSTROPHE,
   Comma = GLFW_KEY_COMMA,
   Minus = GLFW_KEY_MINUS,
   Period = GLFW_KEY_PERIOD,
   Slash = GLFW_KEY_SLASH,
   Num0 = GLFW_KEY_0,
   Num1 = GLFW_KEY_1,
   Num2 = GLFW_KEY_2,
   Num3 = GLFW_KEY_3,
   Num4 = GLFW_KEY_4,
   Num5 = GLFW_KEY_5,
   Num6 = GLFW_KEY_6,
   Num7 = GLFW_KEY_7,
   Num8 = GLFW_KEY_8,
   Num9 = GLFW_KEY_9,
   Semicolon = GLFW_KEY_SEMICOLON,
   Equal = GLFW_KEY_EQUAL,
   A = GLFW_KEY_A,
   B = GLFW_KEY_B,
   C = GLFW_KEY_C,
   D = GLFW_KEY_D,
   E = GLFW_KEY_E,
   F = GLFW_KEY_F,
   G = GLFW_KEY_G,
   H = GLFW_KEY_H,
   I = GLFW_KEY_I,
   J = GLFW_KEY_J,
   K = GLFW_KEY_K,
   L = GLFW_KEY_L,
   M = GLFW_KEY_M,
   N = GLFW_KEY_N,
   O = GLFW_KEY_O,
   P = GLFW_KEY_P,
   Q = GLFW_KEY_Q,
   R = GLFW_KEY_R,
   S = GLFW_KEY_S,
   T = GLFW_KEY_T,
   U = GLFW_KEY_U,
   V = GLFW_KEY_V,
   W = GLFW_KEY_W,
   X = GLFW_KEY_X,
   Y = GLFW_KEY_Y,
   Z = GLFW_KEY_Z,
   LeftBracket = GLFW_KEY_LEFT_BRACKET,
   Backslash = GLFW_KEY_BACKSLASH,
   RightBracket = GLFW_KEY_RIGHT_BRACKET,
   GraveAccent = GLFW_KEY_GRAVE_ACCENT,
   World1 = GLFW_KEY_WORLD_1,
   World2 = GLFW_KEY_WORLD_2,

   // Function keys
   Escape = GLFW_KEY_ESCAPE,
   Enter = GLFW_KEY_ENTER,
   Tab = GLFW_KEY_TAB,
   Backspace = GLFW_KEY_BACKSPACE,
   Insert = GLFW_KEY_INSERT,
   Delete = GLFW_KEY_DELETE,
   Right = GLFW_KEY_RIGHT,
   Left = GLFW_KEY_LEFT,
   Down = GLFW_KEY_DOWN,
   Up = GLFW_KEY_UP,
   PageUp = GLFW_KEY_PAGE_UP,
   PageDown = GLFW_KEY_PAGE_DOWN,
   Home = GLFW_KEY_HOME,
   End = GLFW_KEY_END,
   CapsLock = GLFW_KEY_CAPS_LOCK,
   ScrollLock = GLFW_KEY_SCROLL_LOCK,
   NumLock = GLFW_KEY_NUM_LOCK,
   PrintScreen = GLFW_KEY_PRINT_SCREEN,
   Pause = GLFW_KEY_PAUSE,
   F1 = GLFW_KEY_F1,
   F2 = GLFW_KEY_F2,
   F3 = GLFW_KEY_F3,
   F4 = GLFW_KEY_F4,
   F5 = GLFW_KEY_F5,
   F6 = GLFW_KEY_F6,
   F7 = GLFW_KEY_F7,
   F8 = GLFW_KEY_F8,
   F9 = GLFW_KEY_F9,
   F10 = GLFW_KEY_F10,
   F11 = GLFW_KEY_F11,
   F12 = GLFW_KEY_F12,
   F13 = GLFW_KEY_F13,
   F14 = GLFW_KEY_F14,
   F15 = GLFW_KEY_F15,
   F16 = GLFW_KEY_F16,
   F17 = GLFW_KEY_F17,
   F18 = GLFW_KEY_F18,
   F19 = GLFW_KEY_F19,
   F20 = GLFW_KEY_F20,
   F21 = GLFW_KEY_F21,
   F22 = GLFW_KEY_F22,
   F23 = GLFW_KEY_F23,
   F24 = GLFW_KEY_F24,
   F25 = GLFW_KEY_F25,
   Keypad0 = GLFW_KEY_KP_0,
   Keypad1 = GLFW_KEY_KP_1,
   Keypad2 = GLFW_KEY_KP_2,
   Keypad3 = GLFW_KEY_KP_3,
   Keypad4 = GLFW_KEY_KP_4,
   Keypad5 = GLFW_KEY_KP_5,
   Keypad6 = GLFW_KEY_KP_6,
   Keypad7 = GLFW_KEY_KP_7,
   Keypad8 = GLFW_KEY_KP_8,
   Keypad9 = GLFW_KEY_KP_9,
   KeypadDecimal = GLFW_KEY_KP_DECIMAL,
   KeypadDivide = GLFW_KEY_KP_DIVIDE,
   KeypadMultiply = GLFW_KEY_KP_MULTIPLY,
   KeypadSubtract = GLFW_KEY_KP_SUBTRACT,
   KeypadAdd = GLFW_KEY_KP_ADD,
   KeypadEnter = GLFW_KEY_KP_ENTER,
   KeypadEqual = GLFW_KEY_KP_EQUAL,
   LeftShift = GLFW_KEY_LEFT_SHIFT,
   LeftControl = GLFW_KEY_LEFT_CONTROL,
   LeftAlt = GLFW_KEY_LEFT_ALT,
   LeftSuper = GLFW_KEY_LEFT_SUPER,
   RightShift = GLFW_KEY_RIGHT_SHIFT,
   RightControl = GLFW_KEY_RIGHT_CONTROL,
   RightAlt = GLFW_KEY_RIGHT_ALT,
   RightSuper = GLFW_KEY_RIGHT_SUPER,
   Menu = GLFW_KEY_MENU
};

namespace KeyMod
{
   enum Enum
   {
      None = 0,
      Shift = GLFW_MOD_SHIFT,
      Control = GLFW_MOD_CONTROL,
      Alt = GLFW_MOD_ALT,
      Super = GLFW_MOD_SUPER
   };

   inline bool matches(Enum current, Enum other)
   {
      return (current & other) == current;
   }
}

struct KeyChord
{
   Key key = Key::Unknown;
   KeyMod::Enum mods = KeyMod::None;

   KeyChord() = default;

   KeyChord(Key initialKey, KeyMod::Enum initialMods = KeyMod::None)
      : key(initialKey)
      , mods(initialMods)
   {
   }

   bool operator==(const KeyChord& other) const
   {
      return key == other.key && mods == other.mods;
   }

   bool matches(const KeyChord& other) const
   {
      return key == other.key && KeyMod::matches(mods, other.mods);
   }

   using EnumType = Key;
};

struct KeyAxisChord
{
   Key key = Key::Unknown;
   bool invert = false;

   KeyAxisChord() = default;

   KeyAxisChord(Key initialKey, bool initialInvert = false)
      : key(initialKey)
      , invert(initialInvert)
   {
   }

   bool operator==(const KeyAxisChord& other) const
   {
      return key == other.key && invert == other.invert;
   }

   bool matches(const KeyAxisChord& other) const
   {
      return *this == other;
   }

   using EnumType = Key;
};

enum class MouseButton
{
   First = GLFW_MOUSE_BUTTON_1,
   Second = GLFW_MOUSE_BUTTON_2,
   Third = GLFW_MOUSE_BUTTON_3,
   Fourth = GLFW_MOUSE_BUTTON_4,
   Fifth = GLFW_MOUSE_BUTTON_5,
   Sixth = GLFW_MOUSE_BUTTON_6,
   Seventh = GLFW_MOUSE_BUTTON_7,
   Eighth = GLFW_MOUSE_BUTTON_8,

   Left = First,
   Right = Second,
   Middle = Third
};

struct MouseButtonChord
{
   MouseButton button = MouseButton::Left;
   KeyMod::Enum mods = KeyMod::None;

   MouseButtonChord() = default;

   MouseButtonChord(MouseButton initialButton, KeyMod::Enum initialMods = KeyMod::None)
      : button(initialButton)
      , mods(initialMods)
   {
   }

   bool operator==(const MouseButtonChord& other) const
   {
      return button == other.button && mods == other.mods;
   }

   bool matches(const MouseButtonChord& other) const
   {
      return button == other.button && KeyMod::matches(mods, other.mods);
   }

   using EnumType = MouseButton;
};

enum class CursorAxis
{
   X,
   Y
};

struct CursorAxisChord
{
   CursorAxis axis = CursorAxis::X;
   bool invert = false;

   CursorAxisChord() = default;

   CursorAxisChord(CursorAxis initialAxis, bool initialInvert = false)
      : axis(initialAxis)
      , invert(initialInvert)
   {
   }

   bool operator==(const CursorAxisChord& other) const
   {
      return axis == other.axis && invert == other.invert;
   }

   bool matches(const CursorAxisChord& other) const
   {
      return axis == other.axis && invert == other.invert;
   }

   using EnumType = CursorAxis;
};

enum class GamepadButton
{
   A = GLFW_GAMEPAD_BUTTON_A,
   B = GLFW_GAMEPAD_BUTTON_B,
   X = GLFW_GAMEPAD_BUTTON_X,
   Y = GLFW_GAMEPAD_BUTTON_Y,
   LeftBumper = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
   RightBumper = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
   Back = GLFW_GAMEPAD_BUTTON_BACK,
   Start = GLFW_GAMEPAD_BUTTON_START,
   Guide = GLFW_GAMEPAD_BUTTON_GUIDE,
   LeftThumb = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
   RightThumb = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
   DpadUp = GLFW_GAMEPAD_BUTTON_DPAD_UP,
   DpadRight = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
   DpadDown = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
   DpadLeft = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,

   Cross = A,
   Circle = B,
   Square = X,
   Triangle = Y
};

struct GamepadButtonChord
{
   GamepadButton button = GamepadButton::A;
   int gamepadId = -1;

   GamepadButtonChord() = default;

   GamepadButtonChord(GamepadButton initialButton, int initialGamepadId = 0)
      : button(initialButton)
      , gamepadId(initialGamepadId)
   {
   }

   bool operator==(const GamepadButtonChord& other) const
   {
      return button == other.button && gamepadId == other.gamepadId;
   }

   bool matches(const GamepadButtonChord& other) const
   {
      return *this == other;
   }

   using EnumType = GamepadButton;
};

enum class GamepadAxis
{
   LeftX = GLFW_GAMEPAD_AXIS_LEFT_X,
   LeftY = GLFW_GAMEPAD_AXIS_LEFT_Y,
   RightX = GLFW_GAMEPAD_AXIS_RIGHT_X,
   RightY = GLFW_GAMEPAD_AXIS_RIGHT_Y,
   LeftTrigger = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
   RightTrigger = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER
};

struct GamepadAxisChord
{
   GamepadAxis axis = GamepadAxis::LeftX;
   bool invert = false;
   int gamepadId = -1;

   GamepadAxisChord() = default;

   GamepadAxisChord(GamepadAxis initialAxis, bool initialInvert = false, int initialGamepadId = 0)
      : axis(initialAxis)
      , invert(initialInvert)
      , gamepadId(initialGamepadId)
   {
   }

   bool operator==(const GamepadAxisChord& other) const
   {
      return axis == other.axis && invert == other.invert && gamepadId == other.gamepadId;
   }

   bool matches(const GamepadAxisChord& other) const
   {
      return *this == other;
   }

   using EnumType = GamepadAxis;
};

namespace std
{
   template<>
   struct hash<KeyChord>
   {
      size_t operator()(const KeyChord& keyChord) const
      {
         size_t hash = 0;

         Hash::combine(hash, keyChord.key);
         Hash::combine(hash, keyChord.mods);

         return hash;
      }
   };

   template<>
   struct hash<KeyAxisChord>
   {
      size_t operator()(const KeyAxisChord& keyAxisChord) const
      {
         size_t hash = 0;

         Hash::combine(hash, keyAxisChord.key);
         Hash::combine(hash, keyAxisChord.invert);

         return hash;
      }
   };

   template<>
   struct hash<MouseButtonChord>
   {
      size_t operator()(const MouseButtonChord& mouseButtonChord) const
      {
         size_t hash = 0;

         Hash::combine(hash, mouseButtonChord.button);
         Hash::combine(hash, mouseButtonChord.mods);

         return hash;
      }
   };

   template<>
   struct hash<CursorAxisChord>
   {
      size_t operator()(const CursorAxisChord& cursorAxisChord) const
      {
         size_t hash = 0;

         Hash::combine(hash, cursorAxisChord.axis);
         Hash::combine(hash, cursorAxisChord.invert);

         return hash;
      }
   };

   template<>
   struct hash<GamepadButtonChord>
   {
      size_t operator()(const GamepadButtonChord& gamepadButtonChord) const
      {
         size_t hash = 0;

         Hash::combine(hash, gamepadButtonChord.button);
         Hash::combine(hash, gamepadButtonChord.gamepadId);

         return hash;
      }
   };

   template<>
   struct hash<GamepadAxisChord>
   {
      size_t operator()(const GamepadAxisChord& gamepadAxisChord) const
      {
         size_t hash = 0;

         Hash::combine(hash, gamepadAxisChord.axis);
         Hash::combine(hash, gamepadAxisChord.invert);
         Hash::combine(hash, gamepadAxisChord.gamepadId);

         return hash;
      }
   };
}

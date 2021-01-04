#pragma once

#include "Core/Delegate.h"

#include "Platform/InputTypes.h"

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

template<typename KeyType>
using InputMappings = std::unordered_map<KeyType, std::vector<std::string>>;

template<typename DelegateType>
using InputBindings = std::unordered_map<std::string, DelegateType>;

struct GamepadState
{
   std::array<bool, 15> buttons = {};
   std::array<float, 6> axes = {};
};

class InputManager
{
public:
   // Raw input

   using KeyDelegate = MulticastDelegate<void, KeyChord /* keyChord */, bool /* pressed */>;
   using MouseButtonDelegate = MulticastDelegate<void, MouseButtonChord /* mouseButtonChord */, bool /* pressed */>;
   using CursorAxisDelegate = MulticastDelegate<void, double /* xPos */, double /* yPos */>;
   using GamepadButtonDelegate = MulticastDelegate<void, GamepadButtonChord /* gamepadButtonChord */, bool /* pressed */>;
   using GamepadAxisDelegate = MulticastDelegate<void, GamepadAxisChord /* gamepadAxisChord */, float /* value */>;

   DelegateHandle addKeyDelegate(KeyDelegate::FuncType&& function);
   void removeKeyDelegate(DelegateHandle handle);

   DelegateHandle addMouseButtonDelegate(MouseButtonDelegate::FuncType&& function);
   void removeMouseButtonDelegate(DelegateHandle handle);

   DelegateHandle addCursorAxisDelegate(CursorAxisDelegate::FuncType&& function);
   void removeCursorAxisDelegate(DelegateHandle handle);

   DelegateHandle addGamepadButtonDelegate(GamepadButtonDelegate::FuncType&& function);
   void removeGamepadButtonDelegate(DelegateHandle handle);

   DelegateHandle addGamepadAxisDelegate(GamepadAxisDelegate::FuncType&& function);
   void removeGamepadAxisDelegate(DelegateHandle handle);

   // Mapped input

   using ButtonInputDelegate = MulticastDelegate<void, bool /* pressed */>;
   using AxisInputDelegate = MulticastDelegate<void, float /* value */>;

   void createButtonMapping(std::string_view action, std::optional<KeyChord> keyChord, std::optional<MouseButtonChord> mouseButtonChord, std::optional<GamepadButtonChord> gamepadButtonChord);
   void createButtonMapping(std::string_view action, std::span<const KeyChord> keyChords, std::span<const MouseButtonChord> mouseButtonChords, std::span<const GamepadButtonChord> gamepadButtonChords);
   void destroyButtonMapping(std::string_view action);

   void createAxisMapping(std::string_view action, std::optional<KeyAxisChord> keyAxisChord, std::optional<CursorAxisChord> cursorAxisChord, std::optional<GamepadAxisChord> gamepadAxisChord);
   void createAxisMapping(std::string_view action, std::span<const KeyAxisChord> keyAxisChords, std::span<const CursorAxisChord> cursorAxisChords, std::span<const GamepadAxisChord> gamepadAxisChords);
   void destroyAxisMapping(std::string_view action);

   DelegateHandle bindButtonMapping(std::string_view action, ButtonInputDelegate::FuncType&& function);
   void unbindButtonMapping(DelegateHandle handle);

   DelegateHandle bindAxisMapping(std::string_view action, AxisInputDelegate::FuncType&& function);
   void unbindAxisMapping(DelegateHandle handle);

private:
   friend class Window;

   void init(double cursorX, double cursorY);

   void onKeyEvent(int key, int scancode, int action, int mods);
   void onMouseButtonEvent(int button, int action, int mods);
   void onCursorPosChanged(double xPos, double yPos, bool broadcast);

   void pollEvents();
   void pollGamepad(int gamepadId);

   KeyDelegate keyDelegate;
   MouseButtonDelegate mouseButtonDelegate;
   CursorAxisDelegate cursorAxisDelegate;
   GamepadButtonDelegate gamepadButtonDelegate;
   GamepadAxisDelegate gamepadAxisDelegate;

   InputMappings<KeyChord> keyMappings;
   InputMappings<KeyAxisChord> keyAxisMappings;
   InputMappings<MouseButtonChord> mouseButtonMappings;
   InputMappings<CursorAxisChord> cursorAxisMappings;
   InputMappings<GamepadButtonChord> gamepadButtonMappings;
   InputMappings<GamepadAxisChord> gamepadAxisMappings;

   InputBindings<ButtonInputDelegate> buttonBindings;
   InputBindings<AxisInputDelegate> axisBindings;

   std::unordered_set<KeyChord> heldKeys;

   double lastCursorX = 0.0;
   double lastCursorY = 0.0;
   std::array<GamepadState, 16> gamepadStates = {};
};

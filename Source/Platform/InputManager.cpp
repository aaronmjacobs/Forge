#include "Platform/InputManager.h"

#include "Core/Enum.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <utility>

namespace
{
   template<typename KeyType>
   void createMapping(InputMappings<KeyType>& mappings, const KeyType& value, std::string_view action)
   {
      auto mappingLocation = mappings.find(value);

      if (mappingLocation == mappings.end())
      {
         auto pair = mappings.emplace(value, std::vector<std::string>());
         mappingLocation = pair.first;
      }

      std::vector<std::string>& actions = mappingLocation->second;
      actions.emplace_back(action);
   }

   template<typename KeyType>
   void destroyMapping(InputMappings<KeyType>& mappings, std::string_view action)
   {
      for (auto it = mappings.begin(); it != mappings.end();)
      {
         std::vector<std::string>& actions = it->second;
         actions.erase(std::remove_if(actions.begin(), actions.end(), [action](const std::string& act)
         {
            return act == action;
         }), actions.end());

         if (actions.empty())
         {
            it = mappings.erase(it);
         }
         else
         {
            ++it;
         }
      }
   }

   template<typename DelegateType>
   DelegateHandle bindMapping(InputBindings<DelegateType>& bindings, std::string_view action, typename DelegateType::FuncType&& function)
   {
      auto pair = bindings.emplace(action, DelegateType());
      const auto& bindingLocation = pair.first;
      auto& delegate = bindingLocation->second;
      return delegate.add(std::move(function));
   }

   template<typename DelegateType>
   void unbindMapping(InputBindings<DelegateType>& bindings, DelegateHandle handle)
   {
      for (auto it = bindings.begin(); it != bindings.end();)
      {
         DelegateType& delegate = it->second;
         delegate.remove(handle);

         if (!delegate.isBound())
         {
            it = bindings.erase(it);
         }
         else
         {
            ++it;
         }
      }
   }

   template<typename KeyType, typename DelegateType, typename ValueType>
   void broadcastEvent(const InputMappings<KeyType>& mappings, const InputBindings<DelegateType>& bindings, KeyType key, ValueType value)
   {
      auto mapingLocation = mappings.find(key);
      if (mapingLocation != mappings.end())
      {
         const auto& actions = mapingLocation->second;
         for (const std::string& action : actions)
         {
            auto bindingLocation = bindings.find(action);
            if (bindingLocation != bindings.end())
            {
               const auto& delegate = bindingLocation->second;
               delegate.broadcast(value);
            }
         }
      }
   }

   float applyDeadzone(float value, float lowerDeadzone, float upperDeadzone)
   {
      ASSERT(lowerDeadzone >= 0.0f && lowerDeadzone < 1.0f);
      ASSERT(upperDeadzone >= 0.0f && upperDeadzone < 1.0f);
      ASSERT(lowerDeadzone + upperDeadzone < 1.0f);

      if (value <= lowerDeadzone)
      {
         return 0.0f;
      }
      else if (value >= 1.0f - upperDeadzone)
      {
         return 1.0f;
      }

      float scale = 1.0f / (1.0f - (lowerDeadzone + upperDeadzone));
      float scaledValue = (glm::clamp(value, lowerDeadzone, 1.0f - upperDeadzone) - lowerDeadzone) * scale;

      return glm::clamp(scaledValue, 0.0f, 1.0f);
   }

   glm::vec2 processStickInput(const glm::vec2& values)
   {
      static const float kStickLowerDeadzone = 0.1f;
      static const float kStickUpperDeadzone = 0.01f;

      glm::vec2 processedValues;
      processedValues.x = glm::smoothstep(0.0f, 1.0f, applyDeadzone(glm::abs(values.x), kStickLowerDeadzone, kStickUpperDeadzone)) * glm::sign(values.x);
      processedValues.y = glm::smoothstep(0.0f, 1.0f, applyDeadzone(glm::abs(values.y), kStickLowerDeadzone, kStickUpperDeadzone)) * glm::sign(values.y) * -1.0f;

      return processedValues;
   }

   float processTriggerInput(float value)
   {
      static const float kTriggerLowerDeadzone = 0.0f;
      static const float kTriggerUpperDeadzone = 0.01f;

      return applyDeadzone((value + 1.0f) * 0.5f, kTriggerLowerDeadzone, kTriggerUpperDeadzone);
   }

   std::optional<GamepadState> pollGamepadState(int gamepadId)
   {
      static_assert(GamepadState().buttons.size() == sizeof(GLFWgamepadstate().buttons) / sizeof(*GLFWgamepadstate().buttons), "Gamepad button array size does not match number of GLFW gamepad buttons");
      static_assert(GamepadState().axes.size() == sizeof(GLFWgamepadstate().axes) / sizeof(*GLFWgamepadstate().axes), "Gamepad axis array size does not match number of GLFW gamepad axes");

      GLFWgamepadstate glfwGamepadState = {};
      if (glfwGetGamepadState(gamepadId, &glfwGamepadState))
      {
         GamepadState gamepadState;

         for (std::size_t i = 0; i < gamepadState.buttons.size(); ++i)
         {
            gamepadState.buttons[i] = glfwGamepadState.buttons[i] == GLFW_PRESS;
         }

         glm::vec2 leftStickValues = processStickInput(glm::vec2(glfwGamepadState.axes[Enum::cast(GamepadAxis::LeftX)], glfwGamepadState.axes[Enum::cast(GamepadAxis::LeftY)]));
         glm::vec2 rightStickValues = processStickInput(glm::vec2(glfwGamepadState.axes[Enum::cast(GamepadAxis::RightX)], glfwGamepadState.axes[Enum::cast(GamepadAxis::RightY)]));

         gamepadState.axes[Enum::cast(GamepadAxis::LeftX)] = leftStickValues.x;
         gamepadState.axes[Enum::cast(GamepadAxis::LeftY)] = leftStickValues.y;
         gamepadState.axes[Enum::cast(GamepadAxis::RightX)] = rightStickValues.x;
         gamepadState.axes[Enum::cast(GamepadAxis::RightY)] = rightStickValues.y;
         gamepadState.axes[Enum::cast(GamepadAxis::LeftTrigger)] = processTriggerInput(glfwGamepadState.axes[Enum::cast(GamepadAxis::LeftTrigger)]);
         gamepadState.axes[Enum::cast(GamepadAxis::RightTrigger)] = processTriggerInput(glfwGamepadState.axes[Enum::cast(GamepadAxis::RightTrigger)]);

         return gamepadState;
      }

      return {};
   }
}

void InputManager::init(double cursorX, double cursorY)
{
   lastCursorX = cursorX;
   lastCursorY = cursorY;
}

DelegateHandle InputManager::addKeyDelegate(KeyDelegate::FuncType&& function)
{
   return keyDelegate.add(std::move(function));
}

void InputManager::removeKeyDelegate(DelegateHandle handle)
{
   keyDelegate.remove(handle);
}

DelegateHandle InputManager::addMouseButtonDelegate(MouseButtonDelegate::FuncType&& function)
{
   return mouseButtonDelegate.add(std::move(function));
}

void InputManager::removeMouseButtonDelegate(DelegateHandle handle)
{
   mouseButtonDelegate.remove(handle);
}

DelegateHandle InputManager::addCursorAxisDelegate(CursorAxisDelegate::FuncType&& function)
{
   return cursorAxisDelegate.add(std::move(function));
}

void InputManager::removeCursorAxisDelegate(DelegateHandle handle)
{
   cursorAxisDelegate.remove(handle);
}

DelegateHandle InputManager::addGamepadButtonDelegate(GamepadButtonDelegate::FuncType&& function)
{
   return gamepadButtonDelegate.add(std::move(function));
}

void InputManager::removeGamepadButtonDelegate(DelegateHandle handle)
{
   gamepadButtonDelegate.remove(handle);
}

DelegateHandle InputManager::addGamepadAxisDelegate(GamepadAxisDelegate::FuncType&& function)
{
   return gamepadAxisDelegate.add(std::move(function));
}

void InputManager::removeGamepadAxisDelegate(DelegateHandle handle)
{
   gamepadAxisDelegate.remove(handle);
}

void InputManager::createButtonMapping(std::string_view action, std::optional<KeyChord> keyChord, std::optional<MouseButtonChord> mouseButtonChord, std::optional<GamepadButtonChord> gamepadButtonChord)
{
   if (keyChord)
   {
      createMapping(keyMappings, *keyChord, action);
   }

   if (mouseButtonChord)
   {
      createMapping(mouseButtonMappings, *mouseButtonChord, action);
   }

   if (gamepadButtonChord)
   {
      createMapping(gamepadButtonMappings, *gamepadButtonChord, action);
   }
}

void InputManager::createButtonMapping(std::string_view action, std::span<const KeyChord> keyChords, std::span<const MouseButtonChord> mouseButtonChords, std::span<const GamepadButtonChord> gamepadButtonChords)
{
   for (const KeyChord& keyChord : keyChords)
   {
      createMapping(keyMappings, keyChord, action);
   }

   for (const MouseButtonChord& mouseButtonChord : mouseButtonChords)
   {
      createMapping(mouseButtonMappings, mouseButtonChord, action);
   }

   for (const GamepadButtonChord& gamepadButtonChord : gamepadButtonChords)
   {
      createMapping(gamepadButtonMappings, gamepadButtonChord, action);
   }
}

void InputManager::destroyButtonMapping(std::string_view action)
{
   destroyMapping(keyMappings, action);
   destroyMapping(mouseButtonMappings, action);
   destroyMapping(gamepadButtonMappings, action);
}

void InputManager::createAxisMapping(std::string_view action, std::optional<KeyAxisChord> keyAxisChord, std::optional<CursorAxisChord> cursorAxisChord, std::optional<GamepadAxisChord> gamepadAxisChord)
{
   if (keyAxisChord)
   {
      createMapping(keyAxisMappings, *keyAxisChord, action);
   }

   if (cursorAxisChord)
   {
      createMapping(cursorAxisMappings, *cursorAxisChord, action);
   }

   if (gamepadAxisChord)
   {
      createMapping(gamepadAxisMappings, *gamepadAxisChord, action);
   }
}

void InputManager::createAxisMapping(std::string_view action, std::span<const KeyAxisChord> keyAxisChords, std::span<const CursorAxisChord> cursorAxisChords, std::span<const GamepadAxisChord> gamepadAxisChords)
{
   for (const KeyAxisChord& keyAxisChord : keyAxisChords)
   {
      createMapping(keyAxisMappings, keyAxisChord, action);
   }

   for (const CursorAxisChord& cursorAxisChord : cursorAxisChords)
   {
      createMapping(cursorAxisMappings, cursorAxisChord, action);
   }

   for (const GamepadAxisChord& gamepadAxisChord : gamepadAxisChords)
   {
      createMapping(gamepadAxisMappings, gamepadAxisChord, action);
   }
}

void InputManager::destroyAxisMapping(std::string_view action)
{
   destroyMapping(keyAxisMappings, action);
   destroyMapping(cursorAxisMappings, action);
   destroyMapping(gamepadAxisMappings, action);
}

DelegateHandle InputManager::bindButtonMapping(std::string_view action, ButtonInputDelegate::FuncType&& function)
{
   return bindMapping(buttonBindings, action, std::move(function));
}

void InputManager::unbindButtonMapping(DelegateHandle handle)
{
   unbindMapping(buttonBindings, handle);
}

DelegateHandle InputManager::bindAxisMapping(std::string_view action, AxisInputDelegate::FuncType&& function)
{
   return bindMapping(axisBindings, action, std::move(function));
}

void InputManager::unbindAxisMapping(DelegateHandle handle)
{
   unbindMapping(axisBindings, handle);
}

void InputManager::onKeyEvent(int key, int scancode, int action, int mods)
{
   if (action != GLFW_REPEAT)
   {
      KeyChord keyChord;
      keyChord.key = static_cast<Key>(key);
      keyChord.mods = static_cast<KeyMod::Enum>(mods);

      bool pressed = action == GLFW_PRESS;

      keyDelegate.broadcast(keyChord, pressed);

      broadcastEvent(keyMappings, buttonBindings, keyChord, pressed);

      if (pressed)
      {
         heldKeys.insert(keyChord);
      }
      else
      {
         heldKeys.erase(keyChord);
      }
   }
}

void InputManager::onMouseButtonEvent(int button, int action, int mods)
{
   MouseButtonChord mouseButtonChord;
   mouseButtonChord.button = static_cast<MouseButton>(button);
   mouseButtonChord.mods = static_cast<KeyMod::Enum>(mods);

   bool pressed = action == GLFW_PRESS;

   mouseButtonDelegate.broadcast(mouseButtonChord, pressed);

   broadcastEvent(mouseButtonMappings, buttonBindings, mouseButtonChord, pressed);
}

void InputManager::onCursorPosChanged(double xPos, double yPos, bool broadcast)
{
   static const double kMouseSensitivity = 0.05;

   if (broadcast)
   {
      cursorAxisDelegate.broadcast(xPos, yPos);

      float xDiff = static_cast<float>((xPos - lastCursorX) * kMouseSensitivity);
      float yDiff = static_cast<float>((lastCursorY - yPos) * kMouseSensitivity);

      CursorAxisChord xAxisChord;
      xAxisChord.cursorAxis = CursorAxis::X;

      xAxisChord.invert = false;
      broadcastEvent(cursorAxisMappings, axisBindings, xAxisChord, xDiff);

      xAxisChord.invert = true;
      broadcastEvent(cursorAxisMappings, axisBindings, xAxisChord, -xDiff);

      CursorAxisChord yAxisChord;
      yAxisChord.cursorAxis = CursorAxis::Y;

      yAxisChord.invert = false;
      broadcastEvent(cursorAxisMappings, axisBindings, yAxisChord, yDiff);

      yAxisChord.invert = true;
      broadcastEvent(cursorAxisMappings, axisBindings, yAxisChord, -yDiff);
   }

   lastCursorX = xPos;
   lastCursorY = yPos;
}

void InputManager::pollEvents()
{
   for (const KeyChord& heldKey : heldKeys)
   {
      KeyAxisChord keyAxisChord;
      keyAxisChord.keyChord = heldKey;

      keyAxisChord.invert = false;
      broadcastEvent(keyAxisMappings, axisBindings, keyAxisChord, 1.0f);

      keyAxisChord.invert = true;
      broadcastEvent(keyAxisMappings, axisBindings, keyAxisChord, -1.0f);
   }

   static_assert(decltype(gamepadStates)().size() == GLFW_JOYSTICK_LAST + 1, "Gamepad state array size does not match number of GLFW joysticks");
   for (int gamepadId = 0; gamepadId < gamepadStates.size(); ++gamepadId)
   {
      pollGamepad(gamepadId);
   }
}

void InputManager::pollGamepad(int gamepadId)
{
   const GamepadState& currentGamepadState = gamepadStates[gamepadId];
   std::optional<GamepadState> polledNewGamepadState = pollGamepadState(gamepadId);

   if (polledNewGamepadState)
   {
      const GamepadState& newGamepadState = *polledNewGamepadState;

      for (std::size_t i = 0; i < newGamepadState.buttons.size(); ++i)
      {
         if (currentGamepadState.buttons[i] != newGamepadState.buttons[i])
         {
            GamepadButtonChord gamepadButtonChord;
            gamepadButtonChord.button = static_cast<GamepadButton>(i);
            gamepadButtonChord.gamepadId = gamepadId;

            bool pressed = newGamepadState.buttons[i];

            gamepadButtonDelegate.broadcast(gamepadButtonChord, pressed);

            broadcastEvent(gamepadButtonMappings, buttonBindings, gamepadButtonChord, pressed);
         }
      }

      for (std::size_t i = 0; i < newGamepadState.axes.size(); ++i)
      {
         if (newGamepadState.axes[i] != 0.0f || currentGamepadState.axes[i] != newGamepadState.axes[i])
         {
            GamepadAxisChord gamepadAxisChord;
            gamepadAxisChord.axis = static_cast<GamepadAxis>(i);
            gamepadAxisChord.gamepadId = gamepadId;

            float value = newGamepadState.axes[i];

            gamepadAxisDelegate.broadcast(gamepadAxisChord, value);

            gamepadAxisChord.invert = false;
            broadcastEvent(gamepadAxisMappings, axisBindings, gamepadAxisChord, value);

            gamepadAxisChord.invert = true;
            broadcastEvent(gamepadAxisMappings, axisBindings, gamepadAxisChord, -value);
         }
      }

      gamepadStates[gamepadId] = newGamepadState;
   }
}

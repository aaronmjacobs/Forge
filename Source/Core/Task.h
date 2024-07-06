#pragma once

#include "Core/Hash.h"

#include <chrono>
#include <future>

template<typename T>
class Task
{
public:
   template<typename... Args>
   Task(Args&&... args)
      : future(std::async(std::launch::async, std::forward<Args>(args)...))
   {
   }

   bool isValid() const
   {
      return future.valid();
   }

   bool isDone() const
   {
      return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
   }

   T getResult()
   {
      return future.get();
   }

private:
   std::future<T> future;
};

#pragma once

#include "nsignal.h"

template<typename... Args, typename Obj, typename Ret>
auto connect(NSignal<Args...>& signal,
             Obj* obj,
             Ret(Obj::*method)(Args...))
{
    return signal.connect(
        [obj, method](Args... args)
        {
            (obj->*method)(args...);
        }
        );
}

template<typename Obj, typename Ret>
auto connect(NSignal<>& signal,
             Obj* obj,
             Ret(Obj::*method)())
{
    return signal.connect(
        [obj, method]()
        {
            (obj->*method)();
        }
        );
}

template<typename... Args, typename Obj, typename Ret>
auto connect(NSignal<Args...>& signal,
             Obj* obj,
             Ret(Obj::*method)())
{
    return signal.connect(
        [obj, method](Args...)
        {
            (obj->*method)();
        }
        );
}

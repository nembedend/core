// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

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

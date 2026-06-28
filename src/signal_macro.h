// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#define signals public:
#define slots public:

#define SIGNAL(name, ...) \
private: \
    Signal<__VA_ARGS__> name##Signal; \
    public: \
    Signal<__VA_ARGS__>& name() { return name##Signal; } \
    void name(__VA_ARGS__ args) { name##Signal.Emit(args); }

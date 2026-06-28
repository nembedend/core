// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once
#include <string>
#include <vector>

struct MetaProperty {
    std::string name;
};

struct MetaSignal {
    std::string name;
};

class MetaObject {
public:
    std::vector<MetaProperty> properties;
    std::vector<MetaSignal> signals;

    void registerProperty(const std::string& name)
    {
        properties.push_back({name});
    }

    void registerSignal(const std::string& name)
    {
        signals.push_back({name});
    }
};

// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "nobject.h"
#include "nstring.h"

class NTranslator : public NObject {
public:
    explicit NTranslator(NObject* parent = nullptr);
    virtual ~NTranslator();

    virtual NString translate(const char* context,
                              const char* sourceText,
                              const char* disambiguation = nullptr,
                              int n = -1) const = 0;

    bool isEmpty() const { return m_empty; }

protected:
    bool m_empty;
};

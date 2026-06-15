// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include <cstddef>

class NSignalBus;

class NMetaConnection {
public:
    NMetaConnection() noexcept;
    ~NMetaConnection();

    NMetaConnection(size_t id) noexcept;

    NMetaConnection(NMetaConnection&& other) noexcept;
    NMetaConnection& operator=(NMetaConnection&& other) noexcept;

    NMetaConnection(const NMetaConnection&) = delete;
    NMetaConnection& operator=(const NMetaConnection&) = delete;

    void disconnect() noexcept;
    bool isValid() const noexcept { return m_valid; }
    size_t id() const noexcept { return m_id; }

    bool operator==(const NMetaConnection& other) const noexcept;

private:
    friend class NSignalBus;

    size_t m_id;
    bool m_valid;
};

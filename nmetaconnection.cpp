// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nmetaconnection.h"
#include "nsignalbus.h"

NMetaConnection::NMetaConnection() noexcept
    : m_id(0)
    , m_valid(false)
{
}
NMetaConnection::~NMetaConnection() { disconnect(); }

NMetaConnection::NMetaConnection(size_t id) noexcept
    : m_id(id)
    , m_valid(true)
{
}

NMetaConnection::NMetaConnection(NMetaConnection&& other) noexcept
    : m_id(other.m_id)
    , m_valid(other.m_valid)
{
    other.m_valid = false;
}

NMetaConnection& NMetaConnection::operator=(NMetaConnection&& other) noexcept
{
    if (this != &other) {
        disconnect();
        m_id = other.m_id;
        m_valid = other.m_valid;
        other.m_valid = false;
    }
    return *this;
}

void NMetaConnection::disconnect() noexcept
{
    if (m_valid) {
        m_valid = false;
    }
}

bool NMetaConnection::operator==(const NMetaConnection& other) const noexcept
{
    return m_valid && other.m_valid && m_id == other.m_id;
}

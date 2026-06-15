// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#include "nfile.h"
#include "ndebug.h"

NFile::NFile(const char* path, NObject* parent)
    : NIODevice(parent)
    , m_path(path)
    , m_file(nullptr)
{
}

NFile::~NFile()
{
    close();
}

void NFile::setFileName(const char* path)
{
    m_path = path;
}

bool NFile::open(OpenMode mode)
{
    if (!m_path)
        return false;

    const char* m = modeToString(mode);
    if (!m)
        return false;

    m_file = fopen(m_path, m);
    if (!m_file)
        return false;

    return NIODevice::open(mode);
}

void NFile::close()
{
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
    NIODevice::close();
}

int64_t NFile::bytesAvailable() const
{
    if (!m_file)
        return 0;

    long cur = ftell(m_file);
    fseek(m_file, 0, SEEK_END);
    long end = ftell(m_file);
    fseek(m_file, cur, SEEK_SET);

    return end - cur;
}

int64_t NFile::size() const
{
    if (!m_file)
        return 0;

    long cur = ftell(m_file);
    fseek(m_file, 0, SEEK_END);
    long end = ftell(m_file);
    fseek(m_file, cur, SEEK_SET);

    return end;
}

int64_t NFile::pos() const
{
    if (!m_file)
        return 0;
    return ftell(m_file);
}

bool NFile::seek(int64_t pos)
{
    if (!m_file)
        return false;
    return fseek(m_file, pos, SEEK_SET) == 0;
}

int64_t NFile::readData(char* data, int64_t maxSize)
{
    if (!m_file)
        return -1;

    size_t r = fread(data, 1, maxSize, m_file);
    if (r > 0) {
        readyRead.emitSignal();
    }
    return (int64_t)r;
}

int64_t NFile::writeData(const char* data, int64_t size)
{
    if (!m_file)
        return -1;

    size_t w = fwrite(data, 1, size, m_file);
    if (w > 0) {
        bytesWritten.emitSignal();
        bytesWrittenCount.emitSignal((int64_t)w);
    }
    return (int64_t)w;
}

const char* NFile::modeToString(OpenMode mode)
{
    switch (mode) {
    case ReadOnly:
        return "rb";
    case WriteOnly:
        return "wb";
    case ReadWrite:
        return "rb+";
    default:
        return nullptr;
    }
}

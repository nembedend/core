// Copyright (c) 2026 Chupligin Sergey
//
// This project is licensed under the GNU General Public License v3.0.
//
// Commercial licenses are available for proprietary and closed-source products.
// See COMMERCIAL_LICENSE.md for details.

#pragma once

#include "niodevice.h"
#include <cstdio>

class NFile : public NIODevice {
public:
    explicit NFile(const char* path, NObject* parent = nullptr);
    virtual ~NFile();

    void setFileName(const char* path);
    const char* fileName() const { return m_path; }

    bool open(OpenMode mode) override;
    void close() override;

    int64_t bytesAvailable() const override;
    int64_t size() const override;
    int64_t pos() const override;
    bool seek(int64_t pos) override;

protected:
    int64_t readData(char* data, int64_t maxSize) override;
    int64_t writeData(const char* data, int64_t size) override;

private:
    const char* m_path;
    FILE* m_file;

    const char* modeToString(OpenMode mode);
};

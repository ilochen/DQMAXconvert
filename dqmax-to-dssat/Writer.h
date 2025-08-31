#ifndef WRITER_H
#define WRITER_H

#include "Converter.h"
#include <string>

class Writer {
public:
    static bool write(const DSSATModel& model, const std::string& filename);
};

#endif

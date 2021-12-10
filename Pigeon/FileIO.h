#pragma once

#include "HeapArray.h"
#include <vector>
#include <string>

// Padding is number of bytes to add to end of HeapArray
// Padding bytes are left uninitialised
HeapArray<unsigned char> load_file_blocking(std::string file_path, unsigned int padding=0);

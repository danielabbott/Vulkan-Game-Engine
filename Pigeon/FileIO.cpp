#include "FileIO.h"
#include <fstream>
#include <stdexcept>

using namespace std;

HeapArray<unsigned char> load_file_blocking(string file_path, unsigned int padding)
{

    ifstream file(file_path, ios::ate | ios::binary);

    if (!file.is_open()) {
        throw runtime_error(string("Error opening file: ").append(file_path));
    }

    file.seekg(0, std::ios::end);
    auto sz = file.tellg();

    file.seekg(0, std::ios::beg);


    HeapArray<unsigned char> ha(static_cast<uintptr_t>(sz) + static_cast<uintptr_t>(padding));

    file.read(reinterpret_cast<char *>(ha.data()), sz);


    return ha;
        
}
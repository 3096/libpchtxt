#include <fstream>
#include <iostream>

#include "../pchtxt.hpp"

int main(int argc, char const* argv[]) {
    auto pchtxtInStream = std::fstream(argv[1]);
    auto pchtxtOut = pchtxt::parsePchtxt(pchtxtInStream, std::cout);
    return 0;
}

#include <fstream>
#include <iostream>
#include <sstream>

#include "../pchtxt.hpp"

int main(int argc, char const* argv[]) {
    auto pchtxtInStream = std::fstream(argv[1]);
    auto logSs = std::stringstream{};
    auto pchtxtOut = pchtxt::parsePchtxt(pchtxtInStream, logSs);
    std::cout << logSs.str() << std::endl;
    return 0;
}

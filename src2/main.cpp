#include <cstdlib>

#include "ast.h"
#include "common.h"

const std::string kCmakeCurrentSourceDir = CMAKE_CURRENT_SOURCE_DIR;

int main(int argc, char* argv[])
{
    using namespace snl;
    auto module = ast::MakeSample1();
    return EXIT_SUCCESS;
}

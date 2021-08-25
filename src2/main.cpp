#include <cstdlib>

#include "ast.h"
#include "bst.h"
#include "common.h"
#include "text_ast.h"

const std::string kCmakeCurrentSourceDir = CMAKE_CURRENT_SOURCE_DIR;

int main(int argc, char* argv[])
{
    auto path = kCmakeCurrentSourceDir + "/samples/helloworld.tree";
    if (auto lines = snl::ReadTextFileToLines(path)) {
        // fmt::print("lines: {}\n", *lines);
        if (auto textAst = snl::ParseLinesToTextAst(*lines)) {
            // auto ast = snl::MakeAstFromTextAst(*textAst);
            auto bst = snl::MakeBstFromTextAst(*textAst);
            return EXIT_SUCCESS;
        } else {
            fmt::print("ParseLinesToTextAst failed.\n");
        }
    } else {
        fmt::print("Can't read {}\n", path);
    }
    return EXIT_FAILURE;
}

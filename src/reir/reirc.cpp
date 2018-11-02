//
// Created by kumagi on 18/06/26.
//

#include <fstream>
#include <cmdline.h>

#include "reir/exec/reir_context.hpp"


int main(int argc, char** argv) {
  cmdline::parser a;

  a.add<std::string>("file", 'f', "target reir file", false, "");
  a.add<std::string>("exec", 'e', "execute reir code directly", false, "");

  a.add("version", 'v', "show version");

  // Run parser.
  // It returns only if command line arguments are valid.
  // If arguments are invalid, a parser output error msgs then exit program.
  // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
  a.parse_check(argc, argv);

  if (a.exist("version")) {
    std::cout << "reir-0.1dev" << std::endl;
    return 0;
  }
  if (a.exist("file")) {
    reir::reir_context ctx;
    std::ifstream t(a.get<std::string>("file"));
    if (t.is_open()) {
      std::string code((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());
      ctx.execute(code);
      return 0;
    } else {
      std::cout << "file " << a.get<std::string>("file") << " does not exist\n";
      return 1;
    }
  }
  if (a.exist("exec")) {
    reir::reir_context ctx;
    auto code = a.get<std::string>("exec");
    ctx.execute(code);
    return 0;
  }

  std::cout << "specify -e or -f option to execute some reir\n";
  return 0;
}

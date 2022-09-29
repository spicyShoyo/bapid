#include "bapid.h"
#include <fmt/core.h>
#include <folly/logging/xlog.h>
#include <iostream>

namespace bapid {
namespace {
constexpr std::string_view kLineFmt{"{:>4}"};
}

/*static*/ int BapidMain::run() {
  XLOG(INFO) << fmt::format(kLineFmt, "|");
  return 0;
}
} // namespace bapid

#include "bapid.h"
#include <fmt/core.h>
#include <glog/logging.h>
#include <iostream>

namespace bapid {
namespace {
constexpr std::string_view kLineFmt{"{:>4}"};
}

/*static*/ int BapidMain::run() {
  LOG(INFO) << fmt::format(kLineFmt, "|");
  return 0;
}
} // namespace bapid

#pragma once

#include "src/bapid.h"

namespace bapid {
class BapidMain {
public:
  explicit BapidMain(Bapid::Config &&config);
  void start(folly::SemiFuture<folly::Unit> &&on_serve);
  const Bapid::Config &getConfig();

private:
  Bapid::Config config_;
};

int runBapidMain(int argc, char **argv);
} // namespace bapid

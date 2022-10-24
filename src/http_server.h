#pragma once

#include <folly/logging/xlog.h>
#include <kj/compat/http.h>
#include <unistd.h>

namespace bapid {

class BapidHttpServer final : public kj::HttpService,
                              public kj::TaskSet::ErrorHandler {

public:
  explicit BapidHttpServer(std::string addr);

  kj::Promise<void> request(kj::HttpMethod method, kj::StringPtr url,
                            const kj::HttpHeaders &headers,
                            kj::AsyncInputStream &requestBody,
                            Response &response) override;

  void start();
  void shutdown();

private:
  void serve(int shutdown_out);
  void taskFailed(kj::Exception &&exception) override;

  const std::string addr_;
  kj::Own<kj::HttpHeaderTable> table_;
  int shutdown_in_{};
  std::thread server_thread_{};
};
} // namespace bapid

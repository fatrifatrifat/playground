#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

#include "strategy_signal.grpc.pb.h"
#include "strategy_signal.pb.h"

#include <functional>

namespace quarcc {

class ISignalSource {
public:
  using SignalCallback =
      std::move_only_function<void(const v1::StrategySignal &signal)>;

  virtual ~ISignalSource() = default;

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void setCallback(SignalCallback callback) = 0;

protected:
  SignalCallback callback_;
};

} // namespace quarcc

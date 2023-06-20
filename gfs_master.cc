/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
#include "examples/protos/gfs.grpc.pb.h"
#else
#include "gfs.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using gfs::GetChunkhandleRequest;
using gfs::GetChunkhandleReply;
using gfs::GFSMaster;
using gfs::ListFilesRequest;
using gfs::ListFilesReply;
using gfs::ErrorCode;

ABSL_FLAG(uint16_t, port, 50052, "Master port for the service");

// Logic and data behind the server's behavior.
class GFSMasterImpl final : public GFSMaster::Service {
 public:
  GFSMasterImpl(std::string storage_path) {}

  ~GFSMasterImpl() {}

  Status GetChunkhandle(ServerContext* context, const GetChunkhandleRequest* request,
                        GetChunkhandleReply* reply) override {
    // TODO: implement db manager, and handle query
    return Status::OK;
  }

  Status ListFiles(ServerContext* context, const ListFilesRequest* request,
                   ListFilesReply* reply) override {
    // TODO: implement db manager, and handle query
    return Status::OK;
  }
};

void RunServer(uint16_t port, std::string path) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  GFSMasterImpl service(path);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Master listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  // TODO: should probably handle signals?
  RunServer(absl::GetFlag(FLAGS_port), argv[1]);
  return 0;
}

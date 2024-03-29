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
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/gfs.grpc.pb.h"
#else
#include "gfs.grpc.pb.h"
#endif

ABSL_FLAG(std::string, target1, "localhost:50051", "Server address");
ABSL_FLAG(std::string, target2, "localhost:50052", "Master address");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using gfs::GetChunkhandleRequest;
using gfs::GetChunkhandleReply;
using gfs::ListFilesRequest;
using gfs::ListFilesReply;
using gfs::PingRequest;
using gfs::PingReply;
using gfs::ReadChunkRequest;
using gfs::ReadChunkReply;
using gfs::WriteChunkRequest;
using gfs::WriteChunkReply;
using gfs::GFS;
using gfs::GFSMaster;

class GFSClient {
 public:
  GFSClient(std::shared_ptr<Channel> channel,
            std::shared_ptr<Channel> master_channel)
      : stub_(GFS::NewStub(channel))
      , stub_master_(GFSMaster::NewStub(master_channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string ClientServerPing(const std::string& user) {
    // Data we are sending to the server.
    PingRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    PingReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->ClientServerPing(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  // Client ReadChunk implementation
  std::string ReadChunk(const int chunkhandle, const int offset,
                        const int length) {
    // Data we are sending to the server.
    ReadChunkRequest request;
    request.set_chunkhandle(chunkhandle);
    request.set_offset(offset);
    request.set_length(length);

    // Container for the data we expect from the server.
    ReadChunkReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->ReadChunk(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      if (reply.bytes_read() == 0) {
        return "ReadChunk failed";
      }
      return reply.data();
    } else {
      return "RPC failed";
    }
  }
  
  // Client WriteChunk implementation
  std::string WriteChunk(const int chunkhandle, const std::string data,
                         const int offset) {
    // Data we are sending to the server.
    WriteChunkRequest request;
    request.set_chunkhandle(chunkhandle);
    request.set_data(data);

    // Container for the data we expect from the server.
    WriteChunkReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->WriteChunk(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      std::cout << "WriteChunk wrote " << reply.bytes_written()
                << " bytes" << std::endl;
      return "RPC succeeded";
    } else {
      return "RPC failed";
    }
  }

    // Client GetChunkhandle implementation
  void GetChunkhandle(const std::string& filename, int64_t chunk_id) {
    GetChunkhandleRequest request;
    request.set_filename(filename);
    request.set_chunk_index(chunk_id);

    GetChunkhandleReply reply;
    ClientContext context;
    Status status = stub_master_->GetChunkhandle(&context, request, &reply);
    if (status.ok()) {
      std::cout << "GetChunkhandle file " << filename
                << " chunk id " << chunk_id
                << " got chunkhandle " << reply.chunkhandle() << std::endl;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
  }

  // Client ListFiles implementation
  void ListFiles(const std::string& prefix) {
    ListFilesRequest request;
    request.set_prefix(prefix);

    ListFilesReply reply;
    ClientContext context;
    Status status = stub_master_->ListFiles(&context, request, &reply);
    if (status.ok()) {
      for (const auto& file_metadata : reply.files()) {
        std::cout << "ListFiles filename " << file_metadata.filename()
                  << std::endl;
      }
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
  }
  
 private:
  std::unique_ptr<GFS::Stub> stub_;
  std::unique_ptr<GFSMaster::Stub> stub_master_;
};

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  std::string target_str1 = absl::GetFlag(FLAGS_target1);
  std::string target_str2 = absl::GetFlag(FLAGS_target2);
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  GFSClient gfs_client(
      grpc::CreateChannel(target_str1, grpc::InsecureChannelCredentials()),
      grpc::CreateChannel(target_str2, grpc::InsecureChannelCredentials()));
  std::string user("GFS");
  std::string reply = gfs_client.ClientServerPing(user);
  std::cout << "Client received: " << reply << std::endl;

  std::string data("new data" + std::to_string(0));
  std::string rpc_result = gfs_client.WriteChunk(0, data, 0);
  reply = gfs_client.ReadChunk(0, 0, data.length());
  std::cout << "Client received chunk data: " << reply << std::endl;
  // Test Master
  gfs_client.GetChunkhandle("test", 0);
  return 0;
}

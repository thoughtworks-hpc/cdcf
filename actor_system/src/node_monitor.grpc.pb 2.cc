// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: node_monitor.proto

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>

#include <functional>

#include "node_monitor.grpc.pb.h"
#include "node_monitor.pb.h"

static const char* NodeMonitor_method_names[] = {
    "/NodeMonitor/GetStatus",
    "/NodeMonitor/GetAllNodeStatus",
};

std::unique_ptr<NodeMonitor::Stub> NodeMonitor::NewStub(
    const std::shared_ptr< ::grpc::ChannelInterface>& channel,
    const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr<NodeMonitor::Stub> stub(new NodeMonitor::Stub(channel));
  return stub;
}

NodeMonitor::Stub::Stub(
    const std::shared_ptr< ::grpc::ChannelInterface>& channel)
    : channel_(channel),
      rpcmethod_GetStatus_(NodeMonitor_method_names[0],
                           ::grpc::internal::RpcMethod::NORMAL_RPC, channel),
      rpcmethod_GetAllNodeStatus_(NodeMonitor_method_names[1],
                                  ::grpc::internal::RpcMethod::NORMAL_RPC,
                                  channel) {}

::grpc::Status NodeMonitor::Stub::GetStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::NodeStatus* response) {
  return ::grpc::internal::BlockingUnaryCall(
      channel_.get(), rpcmethod_GetStatus_, context, request, response);
}

void NodeMonitor::Stub::experimental_async::GetStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::NodeStatus* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(),
                                           stub_->rpcmethod_GetStatus_, context,
                                           request, response, std::move(f));
}

void NodeMonitor::Stub::experimental_async::GetStatus(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::NodeStatus* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(),
                                           stub_->rpcmethod_GetStatus_, context,
                                           request, response, std::move(f));
}

void NodeMonitor::Stub::experimental_async::GetStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::NodeStatus* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetStatus_, context, request,
      response, reactor);
}

void NodeMonitor::Stub::experimental_async::GetStatus(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::NodeStatus* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetStatus_, context, request,
      response, reactor);
}

::grpc::ClientAsyncResponseReader< ::NodeStatus>*
NodeMonitor::Stub::AsyncGetStatusRaw(::grpc::ClientContext* context,
                                     const ::google::protobuf::Empty& request,
                                     ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::NodeStatus>::Create(channel_.get(), cq, rpcmethod_GetStatus_, context,
                            request, true);
}

::grpc::ClientAsyncResponseReader< ::NodeStatus>*
NodeMonitor::Stub::PrepareAsyncGetStatusRaw(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::NodeStatus>::Create(channel_.get(), cq, rpcmethod_GetStatus_, context,
                            request, false);
}

::grpc::Status NodeMonitor::Stub::GetAllNodeStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::AllNodeStatus* response) {
  return ::grpc::internal::BlockingUnaryCall(
      channel_.get(), rpcmethod_GetAllNodeStatus_, context, request, response);
}

void NodeMonitor::Stub::experimental_async::GetAllNodeStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::AllNodeStatus* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(
      stub_->channel_.get(), stub_->rpcmethod_GetAllNodeStatus_, context,
      request, response, std::move(f));
}

void NodeMonitor::Stub::experimental_async::GetAllNodeStatus(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::AllNodeStatus* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(
      stub_->channel_.get(), stub_->rpcmethod_GetAllNodeStatus_, context,
      request, response, std::move(f));
}

void NodeMonitor::Stub::experimental_async::GetAllNodeStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::AllNodeStatus* response,
    ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetAllNodeStatus_, context,
      request, response, reactor);
}

void NodeMonitor::Stub::experimental_async::GetAllNodeStatus(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::AllNodeStatus* response,
    ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetAllNodeStatus_, context,
      request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::AllNodeStatus>*
NodeMonitor::Stub::AsyncGetAllNodeStatusRaw(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::AllNodeStatus>::Create(channel_.get(), cq, rpcmethod_GetAllNodeStatus_,
                               context, request, true);
}

::grpc::ClientAsyncResponseReader< ::AllNodeStatus>*
NodeMonitor::Stub::PrepareAsyncGetAllNodeStatusRaw(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::AllNodeStatus>::Create(channel_.get(), cq, rpcmethod_GetAllNodeStatus_,
                               context, request, false);
}

NodeMonitor::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      NodeMonitor_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler<
          NodeMonitor::Service, ::google::protobuf::Empty, ::NodeStatus>(
          std::mem_fn(&NodeMonitor::Service::GetStatus), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      NodeMonitor_method_names[1], ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler<
          NodeMonitor::Service, ::google::protobuf::Empty, ::AllNodeStatus>(
          std::mem_fn(&NodeMonitor::Service::GetAllNodeStatus), this)));
}

NodeMonitor::Service::~Service() {}

::grpc::Status NodeMonitor::Service::GetStatus(
    ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
    ::NodeStatus* response) {
  (void)context;
  (void)request;
  (void)response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status NodeMonitor::Service::GetAllNodeStatus(
    ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
    ::AllNodeStatus* response) {
  (void)context;
  (void)request;
  (void)response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

static const char* NodeActorMonitor_method_names[] = {
    "/NodeActorMonitor/GetNodeActorStatus",
};

std::unique_ptr<NodeActorMonitor::Stub> NodeActorMonitor::NewStub(
    const std::shared_ptr< ::grpc::ChannelInterface>& channel,
    const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr<NodeActorMonitor::Stub> stub(
      new NodeActorMonitor::Stub(channel));
  return stub;
}

NodeActorMonitor::Stub::Stub(
    const std::shared_ptr< ::grpc::ChannelInterface>& channel)
    : channel_(channel),
      rpcmethod_GetNodeActorStatus_(NodeActorMonitor_method_names[0],
                                    ::grpc::internal::RpcMethod::NORMAL_RPC,
                                    channel) {}

::grpc::Status NodeActorMonitor::Stub::GetNodeActorStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::ActorStatus* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(),
                                             rpcmethod_GetNodeActorStatus_,
                                             context, request, response);
}

void NodeActorMonitor::Stub::experimental_async::GetNodeActorStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::ActorStatus* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(
      stub_->channel_.get(), stub_->rpcmethod_GetNodeActorStatus_, context,
      request, response, std::move(f));
}

void NodeActorMonitor::Stub::experimental_async::GetNodeActorStatus(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::ActorStatus* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(
      stub_->channel_.get(), stub_->rpcmethod_GetNodeActorStatus_, context,
      request, response, std::move(f));
}

void NodeActorMonitor::Stub::experimental_async::GetNodeActorStatus(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::ActorStatus* response,
    ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetNodeActorStatus_, context,
      request, response, reactor);
}

void NodeActorMonitor::Stub::experimental_async::GetNodeActorStatus(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::ActorStatus* response,
    ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetNodeActorStatus_, context,
      request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::ActorStatus>*
NodeActorMonitor::Stub::AsyncGetNodeActorStatusRaw(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::ActorStatus>::Create(channel_.get(), cq, rpcmethod_GetNodeActorStatus_,
                             context, request, true);
}

::grpc::ClientAsyncResponseReader< ::ActorStatus>*
NodeActorMonitor::Stub::PrepareAsyncGetNodeActorStatusRaw(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::ActorStatus>::Create(channel_.get(), cq, rpcmethod_GetNodeActorStatus_,
                             context, request, false);
}

NodeActorMonitor::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      NodeActorMonitor_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler<
          NodeActorMonitor::Service, ::google::protobuf::Empty, ::ActorStatus>(
          std::mem_fn(&NodeActorMonitor::Service::GetNodeActorStatus), this)));
}

NodeActorMonitor::Service::~Service() {}

::grpc::Status NodeActorMonitor::Service::GetNodeActorStatus(
    ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
    ::ActorStatus* response) {
  (void)context;
  (void)request;
  (void)response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

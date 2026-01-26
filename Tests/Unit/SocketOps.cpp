// IACore-OSS; The Core Library for All IA Open Source Projects
// Copyright (C) 2026 IAS (ias@iasoft.dev)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <IACore/IATest.hpp>
#include <IACore/SocketOps.hpp>

using namespace IACore;

IAT_BEGIN_BLOCK(Core, SocketOps)

auto test_initialization() -> bool
{
  IAT_CHECK(SocketOps::is_initialized());

  const auto res = SocketOps::initialize();
  IAT_CHECK(res.has_value());

  SocketOps::terminate();

  IAT_CHECK(SocketOps::is_initialized());

  return true;
}

auto test_port_availability() -> bool
{

  const u16 port = 54321;

  (void) SocketOps::is_port_available_tcp(port);
  (void) SocketOps::is_port_available_udp(port);

  return true;
}

auto test_unix_socket_lifecycle() -> bool
{
  const String socket_path = "iatest_ipc.sock";

  SocketOps::unlink_file(socket_path.c_str());

  auto server_res = SocketOps::create_unix_socket();
  IAT_CHECK(server_res.has_value());
  SocketHandle server = *server_res;

  auto bind_res = SocketOps::bind_unix_socket(server, socket_path.c_str());
  if (!bind_res)
  {

    SocketOps::close(server);
    return false;
  }

  auto listen_res = SocketOps::listen(server);
  IAT_CHECK(listen_res.has_value());

  auto client_res = SocketOps::create_unix_socket();
  IAT_CHECK(client_res.has_value());
  SocketHandle client = *client_res;

  auto connect_res = SocketOps::connect_unix_socket(client, socket_path.c_str());
  IAT_CHECK(connect_res.has_value());

  SocketOps::close(client);
  SocketOps::close(server);
  SocketOps::unlink_file(socket_path.c_str());

  return true;
}

auto test_unix_socket_errors() -> bool
{
  const String socket_path = "iatest_missing.sock";

  SocketOps::unlink_file(socket_path.c_str());

  auto client_res = SocketOps::create_unix_socket();
  IAT_CHECK(client_res.has_value());
  SocketHandle client = *client_res;

  auto connect_res = SocketOps::connect_unix_socket(client, socket_path.c_str());
  IAT_CHECK_NOT(connect_res.has_value());

  SocketOps::close(client);

  return true;
}

IAT_BEGIN_TEST_LIST()
IAT_ADD_TEST(test_initialization);
IAT_ADD_TEST(test_port_availability);
IAT_ADD_TEST(test_unix_socket_lifecycle);
IAT_ADD_TEST(test_unix_socket_errors);
IAT_END_TEST_LIST()

IAT_END_BLOCK()

IAT_REGISTER_ENTRY(Core, SocketOps)
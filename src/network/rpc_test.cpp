// Copyright (c)2008-2011, Preferred Infrastructure Inc.
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of Preferred Infrastructure nor the names of other
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <time.h>
#include <gtest/gtest.h>

#include "rpc.h"

#include <vector>
#include <map>

#include "../lang/bind.h"
#include "../lang/ref.h"
#include "../concurrent/thread.h"

using namespace std;
using namespace pfi::lang;
using namespace pfi::concurrent;

RPC_PROC(test_str, string(string));
static string test_str(const string& v){ return v; }
RPC_PROC(test_pair, pair<int,string>(pair<int,string>));
static pair<int,string> test_pair(const pair<int,string>& v){ return v; }
RPC_PROC(test_vec, vector<int>(vector<int>));
static vector<int> test_vec(const vector<int>& v){ return v; }
RPC_PROC(test_map, map<int,int>(map<int,int>));
static map<int, int> test_map(const map<int, int>& v){ return v; }
RPC_PROC(test_set, set<int>(set<int> v));
static set<int> test_set(const set<int>& v){ return v; }

RPC_GEN(1, testrpc, test_str, test_pair, test_vec, test_map, test_set);

static void server_thread(testrpc_server& ser)
{
  ser.set_test_str(&test_str);
  ser.set_test_pair(&test_pair);
  ser.set_test_vec(&test_vec);
  ser.set_test_map(&test_map);
  ser.set_test_set(&test_set);
  ser.serv(31231, 10);
}

TEST(rpc, rpc_test)
{
  testrpc_server ser;
  EXPECT_TRUE(ser.is_stopped());
  thread t(pfi::lang::bind(&server_thread, pfi::lang::ref(ser)));
  t.start();

  sleep(1);
  EXPECT_TRUE(ser.is_running());

  EXPECT_NO_THROW({ testrpc_client cln1("localhost", 31231); });
  {
    testrpc_client cln("localhost", 31231);
    int times = 100;
    for (int t=0;t<times;t++) {
      {
        string v;
        for (int i=0;i<10;i++)
          v+='0'+(rand()%10);
        string r;
        EXPECT_NO_THROW({ r = cln.call_test_str(v); });
        EXPECT_EQ(r.size(), 10U);
        for (int i=0;i<10;i++)
          EXPECT_EQ(r[i], v[i]);
      }
      {
        string vs;
        for (int i=0;i<10;i++)
          vs+='0'+(rand()%10);
        pair<int, string> v = make_pair(rand(), vs);
        pair<int, string> r;
        EXPECT_NO_THROW({ r = cln.call_test_pair(v); });
        EXPECT_EQ(r.first, v.first);
        EXPECT_EQ(r.second, v.second);
      }
      {
        vector<int> v;
        for (int i=0;i<10;i++)
          v.push_back(rand());
        vector<int> r;
        EXPECT_NO_THROW({ r = cln.call_test_vec(v); });
        EXPECT_EQ(r.size(), 10U);
        for (int i=0;i<10;i++)
          EXPECT_EQ(r[i], v[i]);
      }
      {
        map<int, int> v;
        for (int i=0;i<10;i++)
          v[i]=rand();
        map<int, int> r;
        EXPECT_NO_THROW({ r = cln.call_test_map(v); });
        EXPECT_EQ(r.size(), 10U);
        for (int i=0;i<10;i++){
          EXPECT_EQ(r[i], v[i]);
        }
      }
      {
        set<int> v;
        for (int i=0;i<10;i++)
          v.insert(i*100);
        set<int> r;
        EXPECT_NO_THROW({ r = cln.call_test_set(v); });
        EXPECT_EQ(r.size(), 10U);
        int cnt = 0;
        for (set<int>::iterator it=v.begin();it!=v.end();++it){
          EXPECT_EQ(*it, cnt++ * 100);
        }
      }
    }
  }

  ser.stop();
  t.join();
  EXPECT_TRUE(ser.is_stopped());
}

// test for struct and empty vector
struct tstruct {
  int i;
  vector<unsigned char> v;
  
private:
  friend class pfi::data::serialization::access;
  template <class Ar>
  void serialize(Ar& ar) {
    ar & i & v;
  }
};

RPC_PROC(test_struct, tstruct(tstruct));
RPC_GEN(1, test_struct_rpc, test_struct);
static tstruct f_test_struct(tstruct t) { return t; }
static void struct_server_thread(test_struct_rpc_server& ser)
{
  ser.set_test_struct(&f_test_struct);
  ser.serv(31232, 10);
}
TEST(rpc, test_struct)
{
  test_struct_rpc_server ser;
  EXPECT_TRUE(ser.is_stopped());
  thread t(pfi::lang::bind(&struct_server_thread, pfi::lang::ref(ser)));
  t.start();

  sleep(1);
  EXPECT_TRUE(ser.is_running());

  EXPECT_NO_THROW({ test_struct_rpc_client cln1("localhost", 31232); });
  {
    test_struct_rpc_client cln("localhost", 31232);
    tstruct t1;
    t1.i = 9;
    tstruct t2;
    EXPECT_NO_THROW({ t2 = cln.call_test_struct(t1); });
    EXPECT_EQ(t1.i, t2.i);
  }

  ser.stop();
  t.join();
  EXPECT_TRUE(ser.is_stopped());
}


// test rpc::server when port number is 0
struct sserver_args_struct
{
  testrpc_server sv;
  uint16_t arg_port;
};

static void server_thread_for_port_0( sserver_args_struct *sas )
{
  sas->sv.serv(sas->arg_port, 1);
}

TEST(rpc, test_server_port_0)
{
  sserver_args_struct sas;
  sas.sv.set_test_str(&test_str);
  sas.arg_port = 0;
  EXPECT_TRUE(sas.sv.is_stopped());

  pfi::lang::function<void(void)> f = bind(server_thread_for_port_0, &sas);
  thread t(f);
  t.start();
  sleep(1);
  EXPECT_TRUE(sas.sv.is_running());

  uint16_t p0 = sas.sv.port();
  EXPECT_NE(0, p0);

  {
    testrpc_client cln("localhost", p0);
    string v;
    for (int i=0;i<10;i++)
      v+='0'+(rand()%10);
    string r;
    EXPECT_NO_THROW({ r = cln.call_test_str(v); });
    EXPECT_EQ(r.size(), 10U);
    for (int i=0;i<10;i++)
      EXPECT_EQ(r[i], v[i]);
  }

  sas.sv.stop();
  t.join();
  EXPECT_TRUE(sas.sv.is_stopped());
}

// test rpc::server timeout
static void server_thread_for_timeout(test_struct_rpc_server& ser)
{
  ser.set_test_struct(&f_test_struct);
  ser.serv(31233, 1);
}

static void dummy_client_for_timeout()
{
  pfi::network::stream_socket s0;
  s0.connect("localhost", 31233);
  sleep(10);
}

uint64_t get_monotonic_time_micro_sec() {
  timespec tt;
  clock_gettime(CLOCK_MONOTONIC, &tt);
  return ( tt.tv_sec*1000000 + tt.tv_nsec/1000 );
}

TEST(rpc, test_rpc_server_timeout)
{
  double test_timeout_sec = 1.0;
  test_struct_rpc_server ser(test_timeout_sec);
  EXPECT_TRUE(ser.is_stopped());
  thread t(pfi::lang::bind(&server_thread_for_timeout, pfi::lang::ref(ser)));
  t.start();

  sleep(1);
  EXPECT_TRUE(ser.is_running());

  uint64_t time0 = get_monotonic_time_micro_sec();
  thread dummy_client(&dummy_client_for_timeout);
  dummy_client.start();
  dummy_client.detach();

  sleep(1);

  EXPECT_NO_THROW({ test_struct_rpc_client cln1("localhost", 31233); });
  {
    test_struct_rpc_client cln("localhost", 31233);
    tstruct t1;
    t1.i = 9;
    tstruct t2;
    EXPECT_NO_THROW({ t2 = cln.call_test_struct(t1); });
    EXPECT_EQ(t1.i, t2.i);
  }

//  If it is less than 7 seconds, the timeout is considered to be working effectively.
//  If the timeout setting does not work, it will wait for more than 10 seconds.
  uint64_t time1 = get_monotonic_time_micro_sec();
  EXPECT_LE((time1-time0), 7*1000*1000);

  ser.stop();
  t.join();
  EXPECT_TRUE(ser.is_stopped());
}

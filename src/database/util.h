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

#ifndef INCLUDE_GUARD_PFI_DATABASE_UTIL_H_
#define INCLUDE_GUARD_PFI_DATABASE_UTIL_H_

#include <exception>

#include "connection.h"
#include "../lang/safe_bool.h"

namespace pfi{
namespace database{

class committer : public pfi::lang::safe_bool<committer>{
public:
  // committer must not be copied because destructor
  // must be called only once. Do not implement these functions.
  committer(const committer&);
  committer& operator=(const committer&);

public:
  template<typename ThreadingModel>
  committer(pfi::lang::shared_ptr<connection, ThreadingModel>& ptr)
    :conn(ptr.get()){
    conn->begin_transaction();
  }
  committer(connection *conn):conn(conn){
    conn->begin_transaction();
  }

  ~committer(){
    if (std::uncaught_exception()) conn->rollback();
    else conn->commit();
  }

  bool bool_test() const{ return true; }

private:
  connection *conn;
};

} // database
} // pfi

#define transaction(conn) \
  if (committer comm__ = committer(conn))
#endif // #ifndef INCLUDE_GUARD_PFI_DATABASE_UTIL_H_

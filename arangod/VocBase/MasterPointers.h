////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_VOC_BASE_MASTER_POINTERS_H
#define ARANGOD_VOC_BASE_MASTER_POINTERS_H 1

#include "Basics/Common.h"

struct TRI_doc_mptr_t;

namespace arangodb {

class MasterPointers {
  MasterPointers(MasterPointers const&) = delete;
  MasterPointers& operator=(MasterPointers const&) = delete;

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates the headers
  //////////////////////////////////////////////////////////////////////////////

  MasterPointers();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief destroys the headers
  //////////////////////////////////////////////////////////////////////////////

  ~MasterPointers();

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the number of allocated headers
  //////////////////////////////////////////////////////////////////////////////

  uint64_t numAllocated() const { return _nrAllocated; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the memory usage
  //////////////////////////////////////////////////////////////////////////////

  uint64_t memory() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief move an existing header to the end of the linked list
  //////////////////////////////////////////////////////////////////////////////

  void moveBack(TRI_doc_mptr_t*, TRI_doc_mptr_t const*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief unlink an existing header from the linked list, without freeing it
  //////////////////////////////////////////////////////////////////////////////

  void unlink(TRI_doc_mptr_t*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief move an existing header to another position in the linked list
  //////////////////////////////////////////////////////////////////////////////

  void move(TRI_doc_mptr_t*, TRI_doc_mptr_t const*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief relink an existing header into the linked list, at its original
  /// position
  //////////////////////////////////////////////////////////////////////////////

  void relink(TRI_doc_mptr_t*, TRI_doc_mptr_t const*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief request a new header
  //////////////////////////////////////////////////////////////////////////////

  TRI_doc_mptr_t* request(size_t);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief release/free an existing header, putting it back onto the freelist
  //////////////////////////////////////////////////////////////////////////////

  void release(TRI_doc_mptr_t*, bool unlink);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief adjust the total size (called by the collector when changing WAL
  /// markers into datafile markers)
  //////////////////////////////////////////////////////////////////////////////

  void adjustTotalSize(int64_t, int64_t);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the total size of linked headers
  //////////////////////////////////////////////////////////////////////////////

  inline int64_t size() const { return _totalSize; }

 private:

  TRI_doc_mptr_t const* _freelist;  // free headers

  uint64_t _nrAllocated;     // number of allocated headers
  uint64_t _nrLinked;        // number of linked headers
  int64_t _totalSize;      // total size of markers for linked headers

  std::vector<TRI_doc_mptr_t const*> _blocks;
};

}

#endif
////////////////////////////////////////////////////////////////////////////////
/// @brief MVCC transaction manager, distributed
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2014 ArangoDB GmbH, Cologne, Germany
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
/// @author Jan Steemann
/// @author Copyright 2015, ArangoDB GmbH, Cologne, Germany
/// @author Copyright 2011-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_MVCC_DISTRIBUTED_TRANSACTION_MANAGER_H
#define ARANGODB_MVCC_DISTRIBUTED_TRANSACTION_MANAGER_H 1

#include "Basics/Common.h"
#include "Mvcc/Transaction.h"
#include "Mvcc/TransactionId.h"
#include "Mvcc/TransactionManager.h"

struct TRI_vocbase_s;

namespace triagens {
  namespace mvcc {

// -----------------------------------------------------------------------------
// --SECTION--                               class DistributedTransactionManager
// -----------------------------------------------------------------------------

    class DistributedTransactionManager final : public TransactionManager {

// -----------------------------------------------------------------------------
// --SECTION--                                        constructors / destructors
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief create the transaction manager
////////////////////////////////////////////////////////////////////////////////

        DistributedTransactionManager ();

////////////////////////////////////////////////////////////////////////////////
/// @brief destroy the transaction manager
////////////////////////////////////////////////////////////////////////////////

        ~DistributedTransactionManager ();

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief create a top-level transaction
////////////////////////////////////////////////////////////////////////////////

        Transaction* createTransaction (struct TRI_vocbase_s*) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief create a sub-transaction
////////////////////////////////////////////////////////////////////////////////

        Transaction* createSubTransaction (struct TRI_vocbase_s*,
                                           Transaction*) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief lease a transaction
////////////////////////////////////////////////////////////////////////////////

        Transaction* leaseTransaction (TransactionId const&) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief unlease a transaction
////////////////////////////////////////////////////////////////////////////////

        void unleaseTransaction (Transaction*) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief commit a transaction
////////////////////////////////////////////////////////////////////////////////

        void commitTransaction (TransactionId const&) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief roll back a transaction
////////////////////////////////////////////////////////////////////////////////

        void rollbackTransaction (TransactionId const&) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief kill a transaction
////////////////////////////////////////////////////////////////////////////////

        void killTransaction (TransactionId const&) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief get a list of all running transactions
////////////////////////////////////////////////////////////////////////////////

        std::vector<TransactionInfo> runningTransactions (struct TRI_vocbase_s*) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief initializes a transaction with state
////////////////////////////////////////////////////////////////////////////////

        void initializeTransaction (Transaction*) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the status of a transaction
////////////////////////////////////////////////////////////////////////////////

        Transaction::StatusType statusTransaction (TransactionId const&) override final;

////////////////////////////////////////////////////////////////////////////////
/// @brief remove the transaction from the list of running transactions
////////////////////////////////////////////////////////////////////////////////

        void removeRunningTransaction (Transaction*,
                                       bool) override final;

    };

  }
}

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:

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
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_VOC_BASE_REPLICATION_APPLIER_H
#define ARANGOD_VOC_BASE_REPLICATION_APPLIER_H 1

#include "Basics/Common.h"
#include "Basics/ReadWriteLock.h"
#include "Basics/threads.h"
#include "Utils/ReplicationTransaction.h"
#include "VocBase/replication-common.h"
#include "VocBase/voc-types.h"

#include <velocypack/Builder.h>

struct TRI_server_t;
struct TRI_vocbase_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief struct containing a replication apply configuration
////////////////////////////////////////////////////////////////////////////////

class TRI_replication_applier_configuration_t {
  // leftover from struct
 public:
  std::string _endpoint;
  std::string _database;
  std::string _username;
  std::string _password;
  double _requestTimeout;
  double _connectTimeout;
  uint64_t _ignoreErrors;
  uint64_t _maxConnectRetries;
  uint64_t _chunkSize;
  uint64_t _connectionRetryWaitTime;
  uint64_t _idleMinWaitTime;  // 500 * 1000
  uint64_t _idleMaxWaitTime;  // 5 * 500 * 1000
  uint64_t _initialSyncMaxWaitTime;
  uint64_t _autoResyncRetries;
  uint32_t _sslProtocol;
  bool _autoStart;
  bool _adaptivePolling;
  bool _autoResync;
  bool _includeSystem;
  bool _requireFromPresent;
  bool _incremental;
  bool _verbose;
  bool _useCollectionId;
  std::string _restrictType;
  std::unordered_map<std::string, bool> _restrictCollections;

 public:
  TRI_replication_applier_configuration_t(TRI_replication_applier_configuration_t const&) = delete;
  TRI_replication_applier_configuration_t& operator=(TRI_replication_applier_configuration_t const&) = delete;

  TRI_replication_applier_configuration_t();

  ~TRI_replication_applier_configuration_t() {}

  //////////////////////////////////////////////////////////////////////////////
  /// @brief copy an applier configuration
  //////////////////////////////////////////////////////////////////////////////

  void update(TRI_replication_applier_configuration_t const*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief reset the configuration to defaults
  //////////////////////////////////////////////////////////////////////////////

  void reset();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get a VelocyPack representation
  ///        Expects builder to be in an open Object state
  //////////////////////////////////////////////////////////////////////////////

  void toVelocyPack(bool, arangodb::velocypack::Builder&) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get a VelocyPack representation
  //////////////////////////////////////////////////////////////////////////////

  std::shared_ptr<arangodb::velocypack::Builder> toVelocyPack(bool) const;
};

////////////////////////////////////////////////////////////////////////////////
/// @brief struct containing a replication apply error
////////////////////////////////////////////////////////////////////////////////

struct TRI_replication_applier_error_t {
  int _code;
  char* _msg;
  char _time[24];
};

////////////////////////////////////////////////////////////////////////////////
/// @brief state information about replication application
////////////////////////////////////////////////////////////////////////////////

struct TRI_replication_applier_state_t {
  TRI_voc_tick_t _lastProcessedContinuousTick;
  TRI_voc_tick_t _lastAppliedContinuousTick;
  TRI_voc_tick_t _lastAvailableContinuousTick;
  TRI_voc_tick_t _safeResumeTick;
  bool _active;
  bool _preventStart;
  bool _stopInitialSynchronization;
  char* _progressMsg;
  char _progressTime[24];
  TRI_server_id_t _serverId;
  TRI_replication_applier_error_t _lastError;
  uint64_t _failedConnects;
  uint64_t _totalRequests;
  uint64_t _totalFailedConnects;
  uint64_t _totalEvents;
  uint64_t _skippedOperations;
};

////////////////////////////////////////////////////////////////////////////////
/// @brief replication applier
////////////////////////////////////////////////////////////////////////////////

class TRI_replication_applier_t {
 public:
  TRI_replication_applier_t(TRI_server_t*, TRI_vocbase_t*);

  ~TRI_replication_applier_t();

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief pauses and checks whether the apply thread should terminate
  //////////////////////////////////////////////////////////////////////////////

  bool wait(uint64_t);

  bool isTerminated() { return _terminateThread.load(); }

  void setTermination(bool value) { _terminateThread.store(value); }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the database name
  //////////////////////////////////////////////////////////////////////////////

  char const* databaseName() const { return _databaseName.c_str(); }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief test if the replication applier is running
  //////////////////////////////////////////////////////////////////////////////

  bool isRunning() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief block the replication applier from starting
  //////////////////////////////////////////////////////////////////////////////

  int preventStart();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief unblock the replication applier from starting
  //////////////////////////////////////////////////////////////////////////////

  int allowStart();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief check whether the initial synchronization should be stopped
  //////////////////////////////////////////////////////////////////////////////

  bool stopInitialSynchronization();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief stop the initial synchronization
  //////////////////////////////////////////////////////////////////////////////

  void stopInitialSynchronization(bool value);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief start the replication applier
  //////////////////////////////////////////////////////////////////////////////

  int start(TRI_voc_tick_t, bool, TRI_voc_tick_t);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief stop the replication applier
  //////////////////////////////////////////////////////////////////////////////

  int stop(bool);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief stop the applier and "forget" everything
  //////////////////////////////////////////////////////////////////////////////

  int forget();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief shuts down the replication applier
  //////////////////////////////////////////////////////////////////////////////

  int shutdown();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief set the progress with or without a lock
  //////////////////////////////////////////////////////////////////////////////

  void setProgress(char const*, bool);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief register an applier error
  //////////////////////////////////////////////////////////////////////////////

  int setError(int, char const*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get a VelocyPack representation
  ///        Expects builder to be in an open Object state
  //////////////////////////////////////////////////////////////////////////////

  void toVelocyPack(arangodb::velocypack::Builder& builder) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get a VelocyPack representation
  //////////////////////////////////////////////////////////////////////////////

  std::shared_ptr<arangodb::velocypack::Builder> toVelocyPack() const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief increase the starts counter
  //////////////////////////////////////////////////////////////////////////////

  void started() {
    ++_starts;
  }

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief register an applier error
  //////////////////////////////////////////////////////////////////////////////

  int doSetError(int, char const*);

 private:
  std::string _databaseName;
  std::atomic<uint64_t> _starts;

 public:
  TRI_server_t* _server;
  TRI_vocbase_t* _vocbase;
  mutable arangodb::basics::ReadWriteLock _statusLock;
  std::atomic<bool> _terminateThread;
  TRI_replication_applier_state_t _state;
  TRI_replication_applier_configuration_t _configuration;
  TRI_thread_t _thread;
};

////////////////////////////////////////////////////////////////////////////////
/// @brief create a replication applier
////////////////////////////////////////////////////////////////////////////////

TRI_replication_applier_t* TRI_CreateReplicationApplier(TRI_server_t*,
                                                        TRI_vocbase_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief configure the replication applier
////////////////////////////////////////////////////////////////////////////////

int TRI_ConfigureReplicationApplier(
    TRI_replication_applier_t*, TRI_replication_applier_configuration_t const*);

////////////////////////////////////////////////////////////////////////////////
/// @brief get the current replication apply state
////////////////////////////////////////////////////////////////////////////////

int TRI_StateReplicationApplier(TRI_replication_applier_t const*,
                                TRI_replication_applier_state_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief initialize an apply state struct
////////////////////////////////////////////////////////////////////////////////

void TRI_InitStateReplicationApplier(TRI_replication_applier_state_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief destroy an apply state struct
////////////////////////////////////////////////////////////////////////////////

void TRI_DestroyStateReplicationApplier(TRI_replication_applier_state_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief save the replication application state to a file
////////////////////////////////////////////////////////////////////////////////

int TRI_SaveStateReplicationApplier(TRI_vocbase_t*,
                                    TRI_replication_applier_state_t const*,
                                    bool);

////////////////////////////////////////////////////////////////////////////////
/// @brief remove the replication application state file
////////////////////////////////////////////////////////////////////////////////

int TRI_RemoveStateReplicationApplier(TRI_vocbase_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief load the replication application state from a file
////////////////////////////////////////////////////////////////////////////////

int TRI_LoadStateReplicationApplier(TRI_vocbase_t*,
                                    TRI_replication_applier_state_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief remove the replication application configuration file
////////////////////////////////////////////////////////////////////////////////

int TRI_RemoveConfigurationReplicationApplier(TRI_vocbase_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief save the replication application configuration to a file
////////////////////////////////////////////////////////////////////////////////

int TRI_SaveConfigurationReplicationApplier(
    TRI_vocbase_t*, TRI_replication_applier_configuration_t const*, bool);

#endif

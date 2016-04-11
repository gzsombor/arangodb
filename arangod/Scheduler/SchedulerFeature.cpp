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

#include "SchedulerFeature.h"

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include "Logger/LogAppender.h"
#include "ProgramOptions/ProgramOptions.h"
#include "ProgramOptions/Section.h"
#include "RestServer/ServerFeature.h"
#include "Scheduler/SchedulerLibev.h"
#include "Scheduler/SignalTask.h"

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::options;
using namespace arangodb::rest;

Scheduler* SchedulerFeature::SCHEDULER = nullptr;

SchedulerFeature::SchedulerFeature(
    application_features::ApplicationServer* server)
    : ApplicationFeature(server, "Scheduler"),
      _nrSchedulerThreads(2),
      _backend(0),
      _showBackends(false),
      _scheduler(nullptr) {
  setOptional(true);
  requiresElevatedPrivileges(false);
  startsAfter("FileDescriptorsFeature");
  startsAfter("Logger");
  startsAfter("WorkMonitor");
}

void SchedulerFeature::collectOptions(
    std::shared_ptr<options::ProgramOptions> options) {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::collectOptions";

  options->addSection("scheduler", "Configure the I/O scheduler");

  options->addOption("--scheduler.threads",
                     "number of threads for I/O scheduler",
                     new UInt64Parameter(&_nrSchedulerThreads));

  std::unordered_set<uint64_t> backends = {0, 1, 2, 3, 4};

#ifndef _WIN32
  options->addHiddenOption(
      "--scheduler.backend", "1: select, 2: poll, 4: epoll",
      new DiscreteValuesParameter<UInt64Parameter>(&_backend, backends));
#endif

  options->addHiddenOption("--scheduler.show-backends",
                           "show available backends",
                           new BooleanParameter(&_showBackends, false));
}

void SchedulerFeature::validateOptions(
    std::shared_ptr<options::ProgramOptions>) {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::validateOptions";

  if (_showBackends) {
    std::cout << "available io backends are: "
              << SchedulerLibev::availableBackends() << std::endl;
    exit(EXIT_SUCCESS);
  }

  if (_nrSchedulerThreads == 0) {
    LOG(FATAL) << "need at least one I/O thread";
    FATAL_ERROR_EXIT();
  }
}

#ifdef TRI_HAVE_GETRLIMIT
template <typename T>
static std::string StringifyLimitValue(T value) {
  auto max = std::numeric_limits<decltype(value)>::max();
  if (value == max || value == max / 2) {
    return "unlimited";
  }
  return std::to_string(value);
}
#endif

void SchedulerFeature::start() {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::start";

#ifdef TRI_HAVE_GETRLIMIT
  struct rlimit rlim;
  int res = getrlimit(RLIMIT_NOFILE, &rlim);

  if (res == 0) {
    LOG(INFO) << "file-descriptors (nofiles) hard limit is "
              << StringifyLimitValue(rlim.rlim_max) << ", soft limit is "
              << StringifyLimitValue(rlim.rlim_cur);
  }
#endif

  buildScheduler();
  buildControlCHandler();

  bool ok = _scheduler->start(nullptr);

  if (!ok) {
    LOG(FATAL) << "the scheduler cannot be started";
    FATAL_ERROR_EXIT();
  }

  while (!_scheduler->isStarted()) {
    LOG_TOPIC(DEBUG, Logger::STARTUP) << "waiting for scheduler to start";
    usleep(500 * 1000);
  }

  LOG_TOPIC(DEBUG, Logger::STARTUP) << "scheduler has started";
}

void SchedulerFeature::stop() {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::stop";

  static size_t const MAX_TRIES = 10;

  if (_scheduler != nullptr) {
    // unregister all tasks
    _scheduler->unregisterUserTasks();

    // remove all helper tasks
    for (auto& task : _tasks) {
      _scheduler->destroyTask(task);
    }

    _tasks.clear();

    // shutdown the scheduler
    _scheduler->beginShutdown();

    for (size_t count = 0; count < MAX_TRIES && _scheduler->isRunning();
         ++count) {
      LOG_TOPIC(TRACE, Logger::STARTUP) << "waiting for scheduler to stop";
      usleep(100000);
    }

    _scheduler->shutdown();

    // delete the scheduler
    delete _scheduler;
    _scheduler = nullptr;
    SCHEDULER = nullptr;
  }
}

#ifdef _WIN32
bool CtrlHandler(DWORD eventType) {
  bool shutdown = false;
  std::string shutdownMessage;

  switch (eventType) {
    case CTRL_BREAK_EVENT: {
      shutdown = true;
      shutdownMessage = "control-break received";
      break;
    }

    case CTRL_C_EVENT: {
      shutdown = true;
      shutdownMessage = "control-c received";
      break;
    }

    case CTRL_CLOSE_EVENT: {
      shutdown = true;
      shutdownMessage = "window-close received";
      break;
    }

    case CTRL_LOGOFF_EVENT: {
      shutdown = true;
      shutdownMessage = "user-logoff received";
      break;
    }

    case CTRL_SHUTDOWN_EVENT: {
      shutdown = true;
      shutdownMessage = "system-shutdown received";
      break;
    }

    default: {
      shutdown = false;
      break;
    }
  }

  if (shutdown == false) {
    LOG(ERR) << "Invalid CTRL HANDLER event received - ignoring event";
    return true;
  }

  static bool seen = false;

  if (!seen) {
    LOG(INFO) << "" << shutdownMessage << ", beginning shut down sequence";

    if (Scheduler::SCHEDULER != nullptr) {
      Scheduler::SCHEDULER->server()->beginShutdown();
    }

    seen = true;
    return true;
  }

  // ........................................................................
  // user is desperate to kill the server!
  // ........................................................................

  LOG(INFO) << "" << shutdownMessage << ", terminating";
  _exit(EXIT_FAILURE);  // quick exit for windows
  return true;
}

#else

class ControlCTask : public SignalTask {
 public:
  explicit ControlCTask(application_features::ApplicationServer* server)
      : Task("Control-C"), SignalTask(), _server(server), _seen(0) {
    addSignal(SIGINT);
    addSignal(SIGTERM);
    addSignal(SIGQUIT);
  }

 public:
  bool handleSignal() override {
    if (_seen == 0) {
      LOG(INFO) << "control-c received, beginning shut down sequence";
      _server->beginShutdown();
    } else {
      LOG(FATAL) << "control-c received (again!), terminating";
      FATAL_ERROR_EXIT();
    }

    ++_seen;

    return true;
  }

 private:
  application_features::ApplicationServer* _server;
  uint32_t _seen;
};

class HangupTask : public SignalTask {
 public:
  HangupTask() : Task("Hangup"), SignalTask() { addSignal(SIGHUP); }

 public:
  bool handleSignal() override {
    LOG(INFO) << "hangup received, about to reopen logfile";
    LogAppender::reopen();
    LOG(INFO) << "hangup received, reopened logfile";
    return true;
  }
};

#endif

void SchedulerFeature::buildScheduler() {
  _scheduler = new SchedulerLibev(_nrSchedulerThreads, _backend);
  SCHEDULER = _scheduler;
}

void SchedulerFeature::buildControlCHandler() {
#ifdef WIN32
  int result = SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);

  if (result == 0) {
    LOG(WARN) << "unable to install control-c handler";
  }
#else
  Task* controlC = new ControlCTask(server());

  int res = _scheduler->registerTask(controlC);

  if (res == TRI_ERROR_NO_ERROR) {
    _tasks.emplace_back(controlC);
  }
#endif

// hangup handler
#ifndef WIN32
  Task* hangup = new HangupTask();

  int res = _scheduler->registerTask(hangup);

  if (res == TRI_ERROR_NO_ERROR) {
    _tasks.emplace_back(hangup);
  }
#endif
}

void SchedulerFeature::setProcessorAffinity(std::vector<size_t> const& cores) {
#ifdef TRI_HAVE_THREAD_AFFINITY
  size_t j = 0;

  for (uint32_t i = 0; i < _nrSchedulerThreads; ++i) {
    size_t c = cores[j];

    LOG(DEBUG) << "using core " << c << " for scheduler thread " << i;

    _scheduler->setProcessorAffinity(i, c);

    ++j;

    if (j >= cores.size()) {
      j = 0;
    }
  }
#endif
}

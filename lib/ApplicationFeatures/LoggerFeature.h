////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
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

#ifndef APPLICATION_FEATURES_LOGGER_FEATURE_H
#define APPLICATION_FEATURES_LOGGER_FEATURE_H 1

#include "ApplicationFeatures/ApplicationFeature.h"

namespace arangodb {
class LoggerFeature final : public application_features::ApplicationFeature {
 public:
  LoggerFeature(application_features::ApplicationServer* server, bool threaded);

 public:
  void collectOptions(std::shared_ptr<options::ProgramOptions>) override;
  void loadOptions(std::shared_ptr<options::ProgramOptions>) override;
  void validateOptions(std::shared_ptr<options::ProgramOptions>) override;
  void prepare() override;
  void start() override;
  void stop() override;

 public:
  void setBackgrounded(bool backgrounded) { _backgrounded = backgrounded; }
  void disableThreaded() { _threaded = false; }
  void setSupervisor(bool supervisor) { _supervisor = supervisor; }

 private:
  std::vector<std::string> _output;
  std::vector<std::string> _levels;
  bool _useLocalTime;
  std::string _prefix;
  std::string _file;
  bool _lineNumber;
  bool _thread;
  bool _performance;
  bool _keepLogRotate;
  bool _foregroundTty;
  bool _forceDirect;

 private:
  bool _supervisor;
  bool _backgrounded;
  bool _threaded;
};
}

#endif

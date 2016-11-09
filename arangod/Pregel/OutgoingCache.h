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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_OUT_MESSAGE_CACHE_H
#define ARANGODB_OUT_MESSAGE_CACHE_H 1

#include "Basics/Common.h"
#include "Cluster/ClusterInfo.h"

#include "MessageCombiner.h"
#include "MessageFormat.h"
#include "WorkerState.h"

namespace arangodb {
namespace pregel {
/* In the longer run, maybe write optimized implementations for certain use
 cases. For example threaded
 processing */
template <typename V, typename E, typename M>
class OutgoingCache {
 public:
  OutgoingCache(std::shared_ptr<WorkerState<V, E, M>> state);
  ~OutgoingCache();

  void sendMessageTo(std::string const& toValue, M const& data);
  void clear();
  size_t sendMessageCount() const { return _sendMessages; }

  void sendMessages();

 private:
  std::shared_ptr<MessageFormat<M>> _format;
  std::shared_ptr<MessageCombiner<M>> _combiner;

  /// @brief two stage map: shard -> vertice -> message
  std::unordered_map<ShardID, std::unordered_map<std::string, M>> _map;
  std::shared_ptr<WorkerState<V, E, M>> _state;
  ClusterInfo* _ci;
  std::string _baseUrl;
  /// @brief current number of vertices stored

  size_t _containedMessages = 0;
  size_t _sendMessages = 0;
};
}
}
#endif
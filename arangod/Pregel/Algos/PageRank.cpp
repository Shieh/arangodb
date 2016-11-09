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

#include "PageRank.h"
#include "Pregel/Combiners/FloatSumCombiner.h"
#include "Pregel/GraphFormat.h"
#include "Pregel/Utils.h"
#include "Pregel/VertexComputation.h"

#include "Cluster/ClusterInfo.h"
#include "Utils/OperationCursor.h"
#include "Utils/SingleCollectionTransaction.h"
#include "Utils/StandaloneTransactionContext.h"
#include "Utils/Transaction.h"
#include "Vocbase/vocbase.h"

using namespace arangodb;
using namespace arangodb::pregel;
using namespace arangodb::pregel::algos;

struct PageRankGraphFormat : public FloatGraphFormat {
  PageRankGraphFormat(std::string const& field, float vertexNull,
                      float edgeNull)
      : FloatGraphFormat(field, vertexNull, edgeNull) {}
  bool storesEdgeData() const override { return false; }
};

std::shared_ptr<GraphFormat<float, float>> PageRankAlgorithm::inputFormat()
    const {
  return std::make_shared<PageRankGraphFormat>("value", 0, 0);
}

std::shared_ptr<MessageFormat<float>> PageRankAlgorithm::messageFormat() const {
  return std::shared_ptr<FloatMessageFormat>(new FloatMessageFormat());
}

std::shared_ptr<MessageCombiner<float>> PageRankAlgorithm::messageCombiner()
    const {
  return std::shared_ptr<FloatSumCombiner>(new FloatSumCombiner());
}

struct PageRankComputation : public VertexComputation<float, float, float> {
  PageRankComputation() {}
  void compute(std::string const& vertexID,
               MessageIterator<float> const& messages) override {
    
    float* ptr = (float*) mutableVertexData();
    float copy = *ptr;
    //float old = *ptr;
    if (getGlobalSuperstep() > 0) {
      float sum = 0;
      for (const float* msg : messages) {
        sum += *msg;
      }
      *ptr = 0.15 / globalVertexCount() + 0.85 * sum;
    }

    if (getGlobalSuperstep() < 30) {
      EdgeIterator<float> edges = getEdges();
      float val = *ptr / edges.size();
      for (EdgeEntry<float>* edge : edges) {
        sendMessage(edge->toVertexID(), val);
      }
      float diff = fabsf(copy - *ptr);
      aggregateValue("convergence", &diff);
    } else {
      voteHalt();
    }
  }
};

std::shared_ptr<VertexComputation<float, float, float>>
PageRankAlgorithm::createComputation(uint64_t gss) const {
  return std::shared_ptr<PageRankComputation>(new PageRankComputation());
}

void PageRankAlgorithm::aggregators(
    std::vector<std::unique_ptr<Aggregator>>& aggregators) {
  aggregators.push_back(std::make_unique<FloatMaxAggregator>("convergence", 0));
}
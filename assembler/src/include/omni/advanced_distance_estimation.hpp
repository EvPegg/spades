#ifndef ADVANCED_DISTANCE_ESTIMATION_HPP_
#define ADVANCED_DISTANCE_ESTIMATION_HPP_

#include "paired_info.hpp"
#include "omni_utils.hpp"
#include "data_divider.hpp"
#include "peak_finder.hpp"
#include "distance_estimation.hpp"

namespace omnigraph {

template<class Graph>
class AdvancedDistanceEstimator: public DistanceEstimator<Graph> {
private:
	typedef typename Graph::EdgeId EdgeId;
	typedef std::pair<int, int> interval;
	IdTrackHandler<Graph> &int_ids_;

	const static int CUTOFF = 3;
	const static size_t MINIMALPEAKPOINTS = 1; //the minimal number of points in cluster to be considered consistent

	vector<pair<size_t, double> > EstimateEdgePairDistances(vector<PairInfo<EdgeId> > data, vector<size_t> forward) {
vector<pair<size_t, double> > result;
        if (data.size() <= 1) return result;
		std::vector<interval> clusters = divideData(data);
		std::vector<int> peaks;
		size_t cur = 0;
        std::stringstream ss;
        for (size_t i = 0; i < forward.size(); i++){
            ss << forward[i] << " ";
        }
        INFO("Possible distances : " << ss.str());

		for (size_t i = 0; i < clusters.size(); i++) {
            size_t begin = clusters[i].first;
            size_t end = clusters[i].second;
            if (end - begin > MINIMALPEAKPOINTS) {
                while ((cur<forward.size()) && (forward[cur] < rounded_d(data[begin])))
					cur++;
                if (cur == forward.size()) break;
                PeakFinder peakfinder(data, begin, end);
				INFO("Processing window : " << rounded_d(data[begin]) << " " << rounded_d(data[end-1]));
				peakfinder.FFTSmoothing(CUTOFF);
                if ( ( (cur + 1) == forward.size()) || (forward[cur + 1] > rounded_d(data[end - 1]))) {
                    result.push_back(make_pair(forward[cur], 1));
                    INFO("Pair made " << forward[cur]);
                }
				while (cur<forward.size() && forward[cur] <= rounded_d(data[end - 1])) {
					if (peakfinder.isPeak(forward[cur])){ 
                        result.push_back(make_pair(forward[cur], 1));
                        INFO("Pair made " << forward[cur]);
                    }
					cur++;
				}
			}
		}
		return result;
	}

public:
	AdvancedDistanceEstimator(Graph &graph, PairedInfoIndex<Graph> &histogram, IdTrackHandler<Graph> &int_ids, size_t insert_size, size_t read_length, size_t delta, size_t linkage_distance,
			size_t max_distance) : DistanceEstimator<Graph>::DistanceEstimator(graph, histogram, insert_size, read_length, delta, linkage_distance, max_distance), int_ids_(int_ids){    
	        INFO("Advanced Estimator started");
    }

	virtual ~AdvancedDistanceEstimator() {
	}

	virtual void Estimate(PairedInfoIndex<Graph> &result) {
		for (auto iterator = this->histogram_.begin(); iterator != this->histogram_.end(); ++iterator) {
			vector<PairInfo<EdgeId> > data = *iterator;
			EdgeId first = data[0].first;
			EdgeId second = data[0].second;
            int firstNumber =  int_ids_.ReturnIntId(first); 
            int secondNumber =  int_ids_.ReturnIntId(second); 

            INFO("Estimating edges number : " << firstNumber << " " << secondNumber); 

            vector<size_t> forward = this->GetGraphDistances(first, second);
			vector<pair<size_t, double> > estimated = EstimateEdgePairDistances(data, forward);
			vector<PairInfo<EdgeId> > clustered = this->ClusterResult(first, second, estimated);
			this->AddToResult(result, clustered);
		}
	}
};

}

#endif /* ADVANCED_DISTANCE_ESTIMATION_HPP_ */

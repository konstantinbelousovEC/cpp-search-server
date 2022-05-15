#include <vector>
#include "request_queue.h"
#include "document.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
: search_server_(search_server)
, empty_rerspones_(0) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto result = search_server_.FindTopDocuments(raw_query, status);
    UpdateStats(static_cast<int>(result.size()));
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto result = search_server_.FindTopDocuments(raw_query);
    UpdateStats(static_cast<int>(result.size()));
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_rerspones_;
}

void RequestQueue::UpdateStats(int new_responses) {
    if (requests_.size() >= min_in_day_) {
        if (requests_.front().found_docs_amount == 0) {
            empty_rerspones_--;
        }
        requests_.pop_front();
    }
    if (0 == new_responses) {
        empty_rerspones_++;
        requests_.push_back({0});
    } else {
        requests_.push_back({new_responses});
    }
}

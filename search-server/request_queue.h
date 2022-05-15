#pragma once
#include <vector>
#include <deque>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
        UpdateStats(static_cast<int>(result.size()));
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        int found_docs_amount;
    };
    
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int empty_rerspones_;
    
    void UpdateStats(int new_responses);
};

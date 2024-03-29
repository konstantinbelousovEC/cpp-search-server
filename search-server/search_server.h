#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <execution>
#include <string_view>
#include <functional>
#include <iostream>
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy,
                                           const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const
    {
        return FindTopDocuments(raw_query, document_predicate);
    }
    
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy,
                                           const std::string_view raw_query,
                                           DocumentStatus status) const
    {
        return FindTopDocuments(raw_query, status);
    }
    
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy,
                                           const std::string_view raw_query) const
    {
        return FindTopDocuments(raw_query);
    }
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy policy,
                                           const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;
    
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy policy,
                                           const std::string_view raw_query,
                                           DocumentStatus status) const;
    
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy policy,
                                           const std::string_view raw_query) const;
    
    int GetDocumentCount() const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy,
                                                                            const std::string_view raw_query,
                                                                            int document_id) const
    {
        return MatchDocument(raw_query, document_id);
    }
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy,
                                                                            const std::string_view raw_query,
                                                                            int document_id) const;
    
    auto begin() const { return document_ids_.begin(); }
    auto end() const { return document_ids_.end(); }
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id) {
        RemoveDocument(document_id);
    };
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string> stop_words_;
    std::set<std::string, std::less<>> all_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<std::string_view, double> empty_map_ = {};

    bool IsStopWord(const std::string_view word) const;
    static bool IsValidWord(const std::string_view word);
    std::vector<std::string> SplitIntoWordsNoStop(const std::string_view text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text) const;
    Query ParseQuery(std::execution::parallel_policy policy, const std::string_view text) const;
    
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy policy, const Query query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy,
                                                     const std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) != 0) {
            
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
            
        }
    }

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) != 0) {
            
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
            
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy policy, const Query query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> prev_document_to_relevance(100);
    std::for_each(std::execution::par,
                  query.plus_words.begin(),
                  query.plus_words.end(),
                  [this, &prev_document_to_relevance, document_predicate] (const std::string_view word) {
                        if (word_to_document_freqs_.count(word) != 0) {
                            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                                const auto& document_data = documents_.at(document_id);
                                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                                    prev_document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                                }
                            }
                        }
                  });
    
    std::map<int, double> document_to_relevance = prev_document_to_relevance.BuildOrdinaryMap();
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) != 0) {
            
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
            
        }
    }
    
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    
    return matched_documents;
}

#include <string>
#include <cmath>
#include <numeric>
#include <execution>
#include "search_server.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std::string_literals;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWordsView(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0)) throw std::invalid_argument("invalid document id value (less than zero)");
    if ((documents_.count(document_id) > 0)) throw std::invalid_argument("a document with this id already exists");
    
    const auto words = SplitIntoWordsNoStop(document);
    std::for_each(words.begin(),
                  words.end(),
                  [this](const std::string& word){
                        all_words_.insert(word);
                  });
    
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[*(all_words_.find(word))][document_id] += inv_word_count;
    }
    
    std::map<std::string_view, double> word_frequencies;
    for (const std::string& word : words) {
        word_frequencies.insert({*(all_words_.find(word)), word_to_document_freqs_[word][document_id]});
    }
    
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);
    document_to_word_freqs_.insert({document_id, word_frequencies});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    if (documents_.count(document_id) == 0 ) throw std::out_of_range("Incorrect document id"s);
    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words = {};
    
    for (const std::string_view word : query.minus_words) {
        
        if (word_to_document_freqs_.count(word) != 0) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                return {matched_words, documents_.at(document_id).status};
            }
        }
        
    }
    
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) != 0) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(*all_words_.find(word));
            }
        }
    }
    
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy,
                                                                   const std::string_view raw_query, int document_id) const {
    if (documents_.count(document_id) == 0 ) throw std::out_of_range("Incorrect document id"s);
    const auto query = ParseQuery(std::execution::par, raw_query);
    if (any_of(std::execution::par,
               query.minus_words.begin(),
               query.minus_words.end(),
               [this, document_id](const std::string_view word) {
                    return (word_to_document_freqs_.at(word).count(document_id));
               }))
    {
        return {std::vector<std::string_view>{}, documents_.at(document_id).status};
    }
    
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto iter_of_end = std::copy_if(std::execution::par,
                                    query.plus_words.begin(),
                                    query.plus_words.end(),
                                    matched_words.begin(),
                                    [this, document_id](const std::string_view word){
                    if (word_to_document_freqs_.count(word) != 0 &&
                        word_to_document_freqs_.at(word).count(document_id))
                    {
                        return true;
                    };
                        return false;
    });
    
    matched_words.erase(iter_of_end, matched_words.end());
    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    auto last = std::unique(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());
    
    std::transform(std::execution::par, matched_words.begin(), matched_words.end(), matched_words.begin(), [this](std::string_view word){
        return std::string_view(*all_words_.find(std::string(word)));
    });
    
    return {matched_words, documents_.at(document_id).status};
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id) > 0) {
        return document_to_word_freqs_.at(document_id);
    } else {
        return empty_map_;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    if (std::count(document_ids_.begin(), document_ids_.end(), document_id) == 0) return;
    for (const auto& [word, freqs]: document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    std::remove_if(document_ids_.begin(), document_ids_.end(), [document_id](auto &element){
            return element == document_id;
    });
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id) {
    if (std::count(document_ids_.begin(), document_ids_.end(), document_id) == 0) return;
    std::map<std::string_view, double>& ref = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> str_ptr(ref.size());
    
    std::transform(std::execution::par,
                   ref.begin(),
                   ref.end(),
                   str_ptr.begin(),
                   [](auto& element){
                        return (element.first);
                   });
    std::for_each(std::execution::par,
                  str_ptr.begin(),
                  str_ptr.end(),
                  [this, document_id](const std::string_view str){
                        word_to_document_freqs_.at(str).erase(document_id);
                  });
    
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    
    for (auto it = document_ids_.begin(); it < document_ids_.end(); it++) {
        if (*it == document_id) {
            document_ids_.erase(it);
            break;
        };
    }
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(std::string(word)) > 0;
}
bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}
std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string> words;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(std::string(word))) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(std::string(word))) {
            words.push_back(std::string(word));
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query result;
    auto text_container = SplitIntoWordsView(text);
    
    for (const std::string_view word : text_container) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    
    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto last = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last, result.plus_words.end());
    return result;
}

SearchServer::Query SearchServer::ParseQuery(std::execution::parallel_policy policy, const std::string_view text) const {
    Query result;
    
    for (const std::string_view word : SplitIntoWordsView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    
    return result;
}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

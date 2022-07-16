#pragma once
#include "search_server.h"
#include "document.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <iostream>
#include <string_view>
#include <execution>

using namespace std::string_literals;
using namespace std::string_view_literals;

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::pair<Key, Value>& container) {
    std::cout << container.first << ": "s << container.second;
    return out;
}

template <typename Any>
void Print(std::ostream& out, const Any& container) {
  bool first = true;
  for (const auto& element : container) {
      if (first) {
          out << element;
          first = false;
      } else {
          out << ", "s << element;
      }
  }
}

template <typename Element>
std::ostream& operator<<(std::ostream& out, const std::vector<Element>& container) {
    std::cout << "["s;
    Print(out, container);
    std::cout << "]"s;
    return out;
}

template <typename Element>
std::ostream& operator<<(std::ostream& out, const std::set<Element>& container) {
    std::cout << "{"s;
    Print(out, container);
    std::cout << "}"s;
    return out;
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::map<Key, Value>& container) {
    std::cout << "{"s;
    Print(out, container);
    std::cout << "}"s;
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint) {
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Function>
void RunTestImpl(const Function& test_function, const std::string& function_name) {
    test_function();
    std::cerr << function_name << " OK"s << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

// test framework. end.

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
//        std::string_view query = "in"sv;
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddingDocument() {
    const int doc_id = 1;
    const std::string content = "white cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
        
    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    
    {
        const auto found_docs = server.FindTopDocuments("white"s);
        const Document& doc_0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc_0.id, doc_id, "Document was not found by query containing a word from document"s);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("white cat"s);
        const Document& doc_0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc_0.id, doc_id, "Document was not found by query containing several words from document"s);
    }
}

void TestExcludeDocumentsContainingMinusWords() {
    const int doc_id = 1;
    const std::string content = "white cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
        
    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    
    ASSERT_HINT(server.FindTopDocuments("cat -city").empty(), "Search results must exclude documents containing minus words"s);
}

void TestMatchingDocumentsToSearchQuery() {
    const int doc_id = 1;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    
    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
    {
        const auto [result, status] = server.MatchDocument("cat new city"s, doc_id);
        const auto [result_par, status_par] = server.MatchDocument("cat new city"s, doc_id);
        const std::vector<std::string_view> expected_result = {"cat"sv, "city"sv};
        ASSERT_EQUAL_HINT(result, expected_result, "Incorrect result of matching of document to search query"s);
        ASSERT_EQUAL_HINT(result_par, expected_result, "Incorrect result of matching of document to search query in parallel (string as parameter)"s);
        ASSERT(status_par == DocumentStatus::IRRELEVANT);
    }
    
    {
        const auto [result, status] = server.MatchDocument("cat new -city"s, doc_id);
        const auto [result_par, status_par] = server.MatchDocument(std::execution::par, "cat new -city"s, doc_id);
        const std::vector<std::string_view> expected_result = {};
        ASSERT_EQUAL_HINT(result, expected_result, "Incorrect proccessing of presence of negative words"s);
        ASSERT_EQUAL_HINT(result_par, expected_result, "Incorrect proccessing of presence of negative words in parallel"s);
        ASSERT(status_par == DocumentStatus::IRRELEVANT);
    }
    {
        const auto [result, status] = server.MatchDocument("cat cat new city"sv, doc_id);
        const auto [result_par, status_par] = server.MatchDocument(std::execution::par, "cat cat new city"sv, doc_id);
        const std::vector<std::string_view> expected_result = {"cat"sv, "city"sv};
        ASSERT_EQUAL_HINT(result, expected_result, "Incorrect result of matching of document to search query"s);
        ASSERT_EQUAL_HINT(result_par, expected_result, "Incorrect result of paralell matching of document to search query"s);
        ASSERT(status_par == DocumentStatus::IRRELEVANT);
    }
    
}

void TestFoundDocumentsAreSortedByRelevanceInDescendingOrder() {
    const int doc_id_1 = 1;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = {1, 2, 3};
    
    const int doc_id_2 = 2;
    const std::string content_2 = "bold dog under the main city bridge"s;
    const std::vector<int> ratings_2 = {4, 5, 6};
    
    const int doc_id_3 = 3;
    const std::string content_3 = "white rabbit in the new york city"s;
    const std::vector<int> ratings_3 = {7, 8, 9};
    
    SearchServer server("in the under"s);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    
    const auto found_docs = server.FindTopDocuments("cat new city"s);
    ASSERT_HINT(found_docs[0].relevance >= found_docs[1].relevance, "Search results are not sorted in descending order of relevance"s);
    ASSERT_HINT(found_docs[1].relevance >= found_docs[2].relevance, "Search results are not sorted in descending order of relevance"s);
}

void TestCorrectCalculationOfAverageDocumentRating() {
    const int doc_id_1 = 1;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = {5, 7};
    
    const int doc_id_2 = 2;
    const std::string content_2 = "white rabbit in the new york city"s;
    const std::vector<int> ratings_2 = {5, 6};
    
    const int doc_id_3 = 3;
    const std::string content_3 = "bold dog under the main city bridge"s;
    const std::vector<int> ratings_3 = {-4, 5, 6};
    
    const int doc_id_4 = 4;
    const std::string content_4 = "the quick brown fox jumps over the lazy dog"s;
    const std::vector<int> ratings_4 = {-4, -5, -8};
    
    SearchServer server("in the under over"s);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);
    
    
    {
        const auto found_document = server.FindTopDocuments("cat"s)[0];
        const int expected_avg_rating = 6;
        ASSERT_EQUAL_HINT(found_document.rating, expected_avg_rating, "Incorrect result of average rating calculation");
    }

    {
        const auto found_document = server.FindTopDocuments("rabbit"s)[0];
        const int expected_avg_rating = 5;
        ASSERT_EQUAL_HINT(found_document.rating, expected_avg_rating, "Incorrect result of average rating calculation (probably you returns fractional number instead of integer)");
    }
    
    {
        const auto found_document = server.FindTopDocuments("bridge"s)[0];
        const int expected_avg_rating = 2;
        ASSERT_EQUAL_HINT(found_document.rating, expected_avg_rating, "Incorrect result of average rating calculation with positive and negative numbers");
    }
    
    {
        const auto found_document = server.FindTopDocuments("fox"s)[0];
        const int expected_avg_rating = -5;
        ASSERT_EQUAL_HINT(found_document.rating, expected_avg_rating, "Incorrect result of average rating calculation with negative numbers");
    }
}

void TestFilteringSearchResultsByUserPredicat() {
    const int doc_id_1 = 1;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = {10, 20, 30};
        
    const int doc_id_2 = 2;
    const std::string content_2 = "white rabbit in the new york city"s;
    const std::vector<int> ratings_2 = {1, 2, 3};
    
    const int doc_id_3 = 3;
    const std::string content_3 = "bold dog under the main city bridge"s;
    const std::vector<int> ratings_3 = {3, 5, 7};
        
    
    SearchServer server("in the under"s);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, [](int document_id, DocumentStatus status, int rating) { return rating > 10; });
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id_1, "Incorrect filtering by user predicat"s);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id_2, "Incorrect filtering by user predicat"s);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
        ASSERT_HINT(found_docs.empty(), "Incorrect filtering by user predicat"s);
    }
}

void TestSearchingDocumentsWithSpecifiedStatus() {
    const int doc_id_1 = 1;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = {10, 20, 30};
    
    const int doc_id_2 = 2;
    const std::string content_2 = "white rabbit in the new york city"s;
    const std::vector<int> ratings_2 = {5, 10, 15};
    
    SearchServer server("in the under"s);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::IRRELEVANT, ratings_2);
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id_1, "Document not found by user specified status"s);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id_2, "Document not found by user specified status"s);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::BANNED);
        ASSERT_HINT(found_docs.empty(), "Documents must not be found when there are no documents with user specified status"s);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::REMOVED);
        ASSERT_HINT(found_docs.empty(), "Documents must not be found when there are no documents with user specified status"s);
    }
}

void TestCorrectCalculationOfDocumentRelevance() {
    const int doc_id= 1;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    const double expected_relevance_1 = 0.549306;
    
    const int doc_id_2 = 2;
    const std::string content_2 = "white rabbit in the new york city"s;
    const std::vector<int> ratings_2 = {10, 20, 30};
    const double expected_relevance_2 = 0.081093;
        
    const int doc_id_3 = 3;
    const std::string content_3 = "bold dog under the new main city bridge"s;
    const std::vector<int> ratings_3 = {3, 5, 7};
    const double expected_relevance_3 = 0.0579236;
    
    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    
    const auto found_docs = server.FindTopDocuments("cat new city"s);
    ASSERT_HINT(abs(found_docs[0].relevance - expected_relevance_1) < EPSILON, "Incorrect result of document relevance calculation"s);
    ASSERT_HINT(abs(found_docs[1].relevance - expected_relevance_2) < EPSILON, "Incorrect result of document relevance calculation"s);
    ASSERT_HINT(abs(found_docs[2].relevance - expected_relevance_3) < EPSILON, "Incorrect result of document relevance calculation"s);
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeDocumentsContainingMinusWords);
    RUN_TEST(TestMatchingDocumentsToSearchQuery);
    RUN_TEST(TestFoundDocumentsAreSortedByRelevanceInDescendingOrder);
    RUN_TEST(TestCorrectCalculationOfAverageDocumentRating);
    RUN_TEST(TestFilteringSearchResultsByUserPredicat);
    RUN_TEST(TestSearchingDocumentsWithSpecifiedStatus);
    RUN_TEST(TestCorrectCalculationOfDocumentRelevance);
}

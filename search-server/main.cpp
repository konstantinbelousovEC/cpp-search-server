void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddingDocument() {
    const int doc_id = 1;
    const string content = "white cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
        
    SearchServer server;
    server.SetStopWords("in the"s);
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
    const string content = "white cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
        
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    
    ASSERT_HINT(server.FindTopDocuments("cat -city").empty(), "Search results must exclude documents containing minus words"s);
}

void TestMatchingDocumentsToSearchQuery() {
    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    {
        const auto [result, status] = server.MatchDocument("cat new city"s, doc_id);
        const vector<string> expected_result = {"cat"s, "city"s};
        ASSERT_EQUAL_HINT(result, expected_result, "Incorrect result of matching of document to search query"s);
    }
    
    {
        const auto [result, status] = server.MatchDocument("cat new -city"s, doc_id);
        const vector<string> expected_result = {};
        ASSERT_EQUAL_HINT(result, expected_result, "Incorrect proccessing of presence of negative words"s);
    }
    
}

void TestFoundDocumentsAreSortedByRelevanceInDescendingOrder() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = {1, 2, 3};
    
    const int doc_id_2 = 2;
    const string content_2 = "bold dog under the main city bridge"s;
    const vector<int> ratings_2 = {4, 5, 6};
    
    const int doc_id_3 = 3;
    const string content_3 = "white rabbit in the new york city"s;
    const vector<int> ratings_3 = {7, 8, 9};
    
    SearchServer server;
    server.SetStopWords("in the under"s);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    
    const auto found_docs = server.FindTopDocuments("cat new city"s);
    const vector<int> resulting_order_of_docs = {found_docs[0].id, found_docs[1].id, found_docs[2].id};
    const vector<int> expected_order_of_docs = {doc_id_1, doc_id_3, doc_id_2};
    
    ASSERT_EQUAL_HINT(resulting_order_of_docs, expected_order_of_docs, "Search results are not sorted in descending order of relevance"s);
}

void TestCorrectCalculationOfAverageDocumentRating() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_int_res = {5, 7};
    
    const int doc_id_2 = 3;
    const string content_2 = "white rabbit in the new york city"s;
    const vector<int> ratings_fract_res = {5, 6};
    
    SearchServer server;
    server.SetStopWords("in the under"s);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_int_res);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_fract_res);
    
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
}

void TestFilteringSearchResultsByUserPredicat() {
    const int doc_id_1 = 1;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = {10, 20, 30};
        
    const int doc_id_2 = 2;
    const string content_2 = "white rabbit in the new york city"s;
    const vector<int> ratings_2 = {1, 2, 3};
    
    const int doc_id_3 = 3;
    const string content_3 = "bold dog under the main city bridge"s;
    const vector<int> ratings_3 = {3, 5, 7};
        
    
    SearchServer server;
    server.SetStopWords("in the under"s);
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
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = {10, 20, 30};
    
    SearchServer server;
    server.SetStopWords("in the under"s);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc_id_1, "Document not found by user specified status"s);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::BANNED);
        ASSERT_HINT(found_docs.empty(), "Documents must not be found when there are no documents with user specified status"s);
    }
}

void TestCorrectCalculationOfDocumentRelevance() {
    const int doc_id= 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const double expected_relevance = 0.549306;
    
    const int doc_id_2 = 2;
    const string content_2 = "white rabbit in the new york city"s;
    const vector<int> ratings_2 = {10, 20, 30};
        
    const int doc_id_3 = 3;
    const string content_3 = "bold dog under the main city bridge"s;
    const vector<int> ratings_3 = {3, 5, 7};
    
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    
    const auto found_docs = server.FindTopDocuments("cat new city"s);
    ASSERT_HINT(abs(found_docs[0].relevance - expected_relevance) < EPSILON, "Incorrect result of document relevance calculation"s);
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

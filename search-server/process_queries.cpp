#include "process_queries.h"
#include "test_example_functions.h"
#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(
                            const SearchServer& search_server,
                            const std::vector<std::string>& queries)
{
    
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par,
                   queries.begin(),
                   queries.end(),
                   result.begin(),
                   [&search_server](std::string str) {
            return search_server.FindTopDocuments(str);
        });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
                            const SearchServer& search_server,
                            const std::vector<std::string>& queries)
{
    std::vector<Document> result;
    std::vector<std::vector<Document>> all_documents = ProcessQueries(search_server, queries);
    for (const std::vector<Document>& elem : all_documents) {
        for (const Document& document : elem) {
            result.push_back(document);
        }
    }
    return result;
}

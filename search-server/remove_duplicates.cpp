#include "remove_duplicates.h"
#include "search_server.h"
#include <string_view>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string_view>> unique_docs;
    std::vector<int> ids_for_remove;
    for (const int document_id : search_server) {
        std::set<std::string_view> words;
        std::map<std::string_view, double> res = search_server.GetWordFrequencies(document_id);
        for (auto element : res) {
            words.insert(element.first);
        }
        auto success = unique_docs.insert(words).second;
        if (!success) ids_for_remove.push_back(document_id);
    }
    for (int id : ids_for_remove) {
        search_server.RemoveDocument(id);
        std::cout << "Found duplicate document id " << id << std::endl;
    }
}

// changed in several rows

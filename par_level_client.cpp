#include <iostream>
#include <string>
#include <queue>
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <stdexcept>
#include "rapidjson/error/error.h"
#include "rapidjson/reader.h"
#include <future>
#include <thread>


struct ParseException : std::runtime_error, rapidjson::ParseResult {
    ParseException(rapidjson::ParseErrorCode code, const char* msg, size_t offset) : 
        std::runtime_error(msg), 
        rapidjson::ParseResult(code, offset) {}
};

#define RAPIDJSON_PARSE_ERROR_NORETURN(code, offset) \
    throw ParseException(code, #code, offset)

#include <rapidjson/document.h>
#include <chrono>


bool debug = false;

size_t max_threads = 0;

// Updated service URL
const std::string SERVICE_URL = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

// Function to HTTP ecnode parts of URLs. for instance, replace spaces with '%20' for URLs
std::string url_encode(CURL* curl, std::string input) {
  char* out = curl_easy_escape(curl, input.c_str(), input.size());
  std::string s = out;
  curl_free(out);
  return s;
}

// Callback function for writing response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Function to fetch neighbors using libcurl with debugging
std::string fetch_neighbors(CURL* curl, const std::string& node) {

  std::string url = SERVICE_URL + url_encode(curl, node);
  std::string response;

    if (debug)
      std::cout << "Sending request to: " << url << std::endl;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Verbose Logging

    // Set a User-Agent header to avoid potential blocking by the server
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: C++-Client/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
    } else {
      if (debug)
        std::cout << "CURL request successful!" << std::endl;
    }

    // Cleanup
    curl_slist_free_all(headers);

    if (debug) 
      std::cout << "Response received: " << response << std::endl;  // Debug log

    return (res == CURLE_OK) ? response : "{}";
}

// Function to parse JSON and extract neighbors
std::vector<std::string> get_neighbors(const std::string& json_str) {
    std::vector<std::string> neighbors;
    try {
      rapidjson::Document doc;
      doc.Parse(json_str.c_str());
      
      if (doc.HasMember("neighbors") && doc["neighbors"].IsArray()) {
        for (const auto& neighbor : doc["neighbors"].GetArray())
	  neighbors.push_back(neighbor.GetString());
      }
    } catch (const ParseException& e) {
      std::cerr<<"Error while parsing JSON: "<<json_str<<std::endl;
      throw e;
    }
    return neighbors;
}

// Parallel BFS Traversal Function
std::vector<std::vector<std::string>> bfs(CURL* /*unused*/, const std::string& start, int depth) {
    std::vector<std::vector<std::string>> levels;
    std::unordered_set<std::string> visited;

    levels.push_back({start});
    visited.insert(start);

    for (int d = 0; d < depth; d++) {
        if (debug) std::cout << "starting level: " << d << "\n";

        levels.push_back({});
        std::vector<std::future<std::vector<std::string>>> futures;

        for (const std::string& s : levels[d]) {
    // if we already have max_threads running, wait for one to finish
    if (max_threads && futures.size() >= max_threads) {
        futures.front().wait();
        futures.erase(futures.begin());
    }

    futures.push_back(std::async(std::launch::async, [s]() {
        CURL* curl_local = curl_easy_init();
        if (!curl_local) throw std::runtime_error("Failed to init CURL");
        std::string json = fetch_neighbors(curl_local, s);
        curl_easy_cleanup(curl_local);
        return get_neighbors(json);
    }));
}

        // Collect results
        for (auto& fut : futures) {
            try {
                for (const auto& neighbor : fut.get()) {
                    if (!visited.count(neighbor)) {
                        visited.insert(neighbor);
                        levels[d + 1].push_back(neighbor);
                    }
                }
            } catch (const ParseException& e) {
                std::cerr << "Error while fetching neighbors in parallel task\n";
                throw;
            }
        }
    }
    return levels;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <node_name> <depth> [max_threads]\n";
        return 1;
    }

    std::string start_node = argv[1];
    int depth;
    try {
        depth = std::stoi(argv[2]);
    } catch (const std::exception&) {
        std::cerr << "Error: Depth must be an integer.\n";
        return 1;
    }

    if (argc == 4) {
        try {
            max_threads = std::stoul(argv[3]);
            if (max_threads == 0) {
                std::cerr << "Error: max_threads must be > 0\n";
                return 1;
            }
        } catch (const std::exception&) {
            std::cerr << "Error: max_threads must be an integer.\n";
            return 1;
        }
    } else {
        max_threads = std::thread::hardware_concurrency();
        if (max_threads == 0) max_threads = 4;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL\n";
        return -1;
    }

    const auto start{std::chrono::steady_clock::now()};

    for (const auto& n : bfs(curl, start_node, depth)) {
        for (const auto& node : n)
            std::cout << "- " << node << "\n";
        std::cout << n.size() << "\n";
    }

    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    std::cout << "Time to crawl: " << elapsed_seconds.count() << "s\n";

    curl_easy_cleanup(curl);
    return 0;
}
#pragma once

#include <drogon/HttpClient.h>
#include <drogon/utils/coroutine.h>
#include <nlohmann/json.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <format>

#include "../domain/RepoProcessor.hpp"

using namespace drogon;

class GitHubService {
public:
    struct TreeItem {
        std::string path;
        long size;
    };

    static Task<std::vector<std::string>> get_branches(const std::string& owner, const std::string& repo) {
        auto client = HttpClient::newHttpClient("https://api.github.com");
        auto req = HttpRequest::newHttpRequest();
        req->setPath(std::format("/repos/{}/{}/branches", owner, repo));
        req->addHeader("User-Agent", "GitHasher-App"); 

        auto response = co_await client->sendRequestCoro(req);
        
        if (response->getStatusCode() != k200OK) {
            throw std::runtime_error(std::format("GitHub API Error: {}", response->getBody()));
        }

        auto json = nlohmann::json::parse(response->getBody());
        std::vector<std::string> branches;
        for (const auto& item : json) {
            branches.push_back(item["name"]);
        }
        co_return branches;
    }

    static Task<std::vector<TreeItem>> get_tree(const std::string& owner, const std::string& repo, const std::string& branch) {
        auto client = HttpClient::newHttpClient("https://api.github.com");
        auto req = HttpRequest::newHttpRequest();
        req->setPath(std::format("/repos/{}/{}/git/trees/{}?recursive=1", owner, repo, branch));
        req->addHeader("User-Agent", "GitHasher-App");

        auto response = co_await client->sendRequestCoro(req);
        
        if (response->getStatusCode() != k200OK) {
            throw std::runtime_error("Failed to fetch repository tree");
        }

        auto json = nlohmann::json::parse(response->getBody());
        std::vector<TreeItem> tree;
        
        if (json.contains("tree") && json["tree"].is_array()) {
            for (const auto& item : json["tree"]) {
                if (item["type"] == "blob") {
                    tree.push_back({
                        item["path"].get<std::string>(),
                        item.value("size", 0L)
                    });
                }
            }
        }
        co_return tree;
    }

    static Task<std::vector<gh::domain::FileEntry>> download_and_extract(
        std::string owner, std::string repo, std::string branch) {
        
        auto client = HttpClient::newHttpClient("https://github.com");
        auto req = HttpRequest::newHttpRequest();
        req->setPath(std::format("/{}/{}/archive/refs/heads/{}.tar.gz", owner, repo, branch));
        req->addHeader("User-Agent", "GitHasher-App");

        auto response = co_await client->sendRequestCoro(req);

        if (response->getStatusCode() == k302Found || response->getStatusCode() == k301MovedPermanently) {
            std::string new_url = response->getHeader("Location");
            if (new_url.empty()) throw std::runtime_error("Redirect with no Location header");

            auto client2 = HttpClient::newHttpClient(new_url);
            auto req2 = HttpRequest::newHttpRequest();
            req2->addHeader("User-Agent", "GitHasher-App");

            size_t protocol = new_url.find("://");
            if (protocol != std::string::npos) {
                size_t path_start = new_url.find('/', protocol + 3);
                if (path_start != std::string::npos) {
                    req2->setPath(new_url.substr(path_start));
                }
            } else {
                req2->setPath(new_url);
            }

            response = co_await client2->sendRequestCoro(req2);
        }

        if (response->getStatusCode() != k200OK) {
             throw std::runtime_error("Failed to download repository archive. Status: " + 
                std::to_string(response->getStatusCode()));
        }

        std::string_view tar_data = response->getBody();
        if (tar_data.empty()) {
            throw std::runtime_error("Empty archive data received");
        }

        std::vector<gh::domain::FileEntry> files;
        
        struct archive *a = archive_read_new();
        archive_read_support_filter_gzip(a);
        archive_read_support_format_tar(a);
        
        if (archive_read_open_memory(a, tar_data.data(), tar_data.size()) != ARCHIVE_OK) {
            std::string err = archive_error_string(a);
            archive_read_free(a);
            throw std::runtime_error("Failed to initialize libarchive: " + err);
        }

        struct archive_entry *entry;
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            if (archive_entry_filetype(entry) != AE_IFREG) continue;

            std::string path = archive_entry_pathname(entry);
            size_t slash_pos = path.find('/');
            if (slash_pos != std::string::npos) path = path.substr(slash_pos + 1);

            size_t size = archive_entry_size(entry);
            std::string content(size, 0);
            archive_read_data(a, content.data(), size);

            files.push_back({path, content});
        }
        archive_read_free(a);
        co_return files;
    }
};

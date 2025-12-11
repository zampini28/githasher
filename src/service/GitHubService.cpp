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

    static Task<std::vector<gh::domain::FileEntry>> download_and_extract(
        std::string owner, std::string repo, std::string branch) {
        
        auto client = HttpClient::newHttpClient("https://github.com");
        auto req = HttpRequest::newHttpRequest();

        req->setPath(std::format("/{}/{}/archive/refs/heads/{}.tar.gz", owner, repo, branch));
        req->addHeader("User-Agent", "GitHasher-App");

        auto response = co_await client->sendRequestCoro(req);

        if (response->getStatusCode() != k200OK && response->getStatusCode() != k302Found) {
             throw std::runtime_error("Failed to download repository archive");
        }

        std::string_view tar_data = response->getBody();
        
        std::vector<gh::domain::FileEntry> files;
        
        struct archive *a = archive_read_new();
        archive_read_support_filter_gzip(a);
        archive_read_support_format_tar(a);
        
        int r = archive_read_open_memory(a, tar_data.data(), tar_data.size());
        if (r != ARCHIVE_OK) {
            throw std::runtime_error("Failed to initialize libarchive");
        }

        struct archive_entry *entry;
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            if (archive_entry_filetype(entry) != AE_IFREG) continue;

            std::string path = archive_entry_pathname(entry);
            
            size_t slash_pos = path.find('/');
            if (slash_pos != std::string::npos) {
                path = path.substr(slash_pos + 1);
            }

            size_t size = archive_entry_size(entry);
            std::string content(size, 0);
            archive_read_data(a, content.data(), size);

            files.push_back({path, content});
        }

        archive_read_free(a);
        co_return files;
    }
};

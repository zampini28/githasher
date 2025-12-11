#include <drogon/HttpController.h>

#include "../service/GitHubService.cpp" 

using namespace drogon;

class HashController : public HttpController<HashController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HashController::getBranches, "/api/branches", Get);
        ADD_METHOD_TO(HashController::getTree, "/api/tree", Get);
        ADD_METHOD_TO(HashController::previewAndHash, "/api/process", Post);
    METHOD_LIST_END

    Task<void> getBranches(HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback) {
        auto owner = req->getParameter("owner");
        auto repo = req->getParameter("repo");
        try {
            auto branches = co_await GitHubService::get_branches(owner, repo);
            Json::Value ret;
            for(const auto& b : branches) ret.append(b);
            callback(HttpResponse::newHttpJsonResponse(ret));
        } catch (const std::exception& e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(e.what());
            callback(resp);
        }
    }

    Task<void> getTree(HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback) {
        auto owner = req->getParameter("owner");
        auto repo = req->getParameter("repo");
        auto branch = req->getParameter("branch");
        try {
            auto tree_items = co_await GitHubService::get_tree(owner, repo, branch);
            Json::Value ret;
            for(const auto& item : tree_items) {
                Json::Value j_item;
                j_item["path"] = item.path;
                j_item["size"] = (Json::UInt64)item.size;
                ret.append(j_item);
            }
            callback(HttpResponse::newHttpJsonResponse(ret));
        } catch (const std::exception& e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(e.what());
            callback(resp);
        }
    }

    Task<void> previewAndHash(HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback) {
        auto json = *req->getJsonObject();
        std::string owner = json["owner"].asString();
        std::string repo = json["repo"].asString();
        std::string branch = json["branch"].asString();
        
        std::vector<std::string> selected_files;
        if (json.isMember("files") && json["files"].isArray()) {
            for (const auto& f : json["files"]) selected_files.push_back(f.asString());
        }

        auto downloaded = co_await GitHubService::download_and_extract(owner, repo, branch);
        auto result_exp = gh::domain::RepoProcessor::process_repository(downloaded, selected_files);

        if (result_exp) {
            Json::Value ret;
            ret["preview"] = result_exp->preview_content;
            ret["hash"] = result_exp->sha512_hash;
            ret["count"] = (Json::UInt64)result_exp->files_processed;
            callback(HttpResponse::newHttpJsonResponse(ret));
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k400BadRequest);
            resp->setBody(result_exp.error());
            callback(resp);
        }
    }
};

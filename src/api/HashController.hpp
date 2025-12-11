#include <drogon/HttpController.h>

#include "../domain/RepoProcessor.hpp"
#include "../service/GitHubService.cpp"

using namespace drogon;

class HashController : public HttpController<HashController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HashController::getBranches, "/api/branches", Get);
        ADD_METHOD_TO(HashController::previewAndHash, "/api/process", Post);
    METHOD_LIST_END

    Task<void> getBranches(HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback) {
        auto owner = req->getParameter("owner");
        auto repo = req->getParameter("repo");

        try {
            auto branches = co_await GitHubService::get_branches(owner, repo);
            Json::Value ret;
            for(const auto& b : branches) ret.append(b);
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
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
        std::string inc = json.get("include", ".*").asString();
        std::string exc = json.get("exclude", "").asString();

        auto files = co_await GitHubService::download_and_extract(owner, repo, branch);
        
        auto result_exp = gh::domain::RepoProcessor::process_repository(files, inc, exc);

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

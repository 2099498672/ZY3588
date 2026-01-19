#include "util/JsonHelper.h"
#include "json/json.h"

std::string JsonHelper::lastError_;

bool JsonHelper::parse(const std::string& jsonStr, Json::Value& root) {
    Json::Reader reader;
    bool success = reader.parse(jsonStr, root);
    if (!success) {
        lastError_ = reader.getFormattedErrorMessages();
    } else {
        lastError_.clear();
    }
    return success;
}

std::string JsonHelper::stringify(const Json::Value& root) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = ""; // 不缩进，节省空间
    return Json::writeString(writer, root);
}

std::string JsonHelper::getError() {
    return lastError_;
}

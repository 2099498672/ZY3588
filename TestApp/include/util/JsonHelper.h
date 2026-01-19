#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include "json/json.h"
#include <string>

class JsonHelper {
public:
    /**
     * 解析JSON字符串
     * @param jsonStr JSON字符串
     * @param root 解析结果存储
     * @return 是否解析成功
     */
    static bool parse(const std::string& jsonStr, Json::Value& root);

    /**
     * 将JSON值转换为字符串
     * @param root 要转换的JSON值
     * @return 转换后的字符串
     */
    static std::string stringify(const Json::Value& root);

    /**
     * 获取最后一次解析错误信息
     * @return 错误信息
     */
    static std::string getError();

private:
    static std::string lastError_;  // 最后一次错误信息
};

#endif // JSON_HELPER_H

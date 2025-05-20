#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../poll/sqlconnpool.h"

class HttpRequest{
public:
    enum PARSE_STATE{
        REQUEST_LINE,//请求行
        HEADERS,
        BODY,
        FINISH,
    };

    HttpRequest() {Init();}
    ~HttpRequest() = default;

    void Init();
    bool Parse(Buffer& buffer);

    std::string Path() const;//用于只读访问，安全，不会改变对象内部状态。
    std::string & Path();//（返回引用）用于需要修改成员变量的场景，效率高，但要注意不要返回局部变量的引用。
    std::string Method() const;
    std::string version();
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine(const std::string& line);//解析请求行
    void ParseHeader(const std::string& line);//解析请求头
    void ParseBody(const std::string& line);//解析请求体

    void ParsePath_();//解析路径
    void ParsePost_();//解析Post事件
    void ParseFromUrlencoded_();//从url处解析编码
 
    //表示这是一个静态成员函数 不依赖于类的对象：不需要访问或修改某个 HttpRequest 对象的成员变量。你可以直接用 HttpRequest::UserVerify(...) 调用，
    static bool UserVerify(const std::string& user, const std::string& passwd,bool isLogin);//用户验证
    
    PARSE_STATE state_;//解析状态
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string,std::string> headers_;//请求头
    std::unordered_map<std::string,std::string> post_;//请求体

    static const std::unordered_set<std::string> DEFAULT_HTML;//默认文件
    static const std::unordered_map<std::string,int> DEFAULT_HTML_TAG;//默认文件类型
    static int ConverHex(char ch);//十六进制转为十进制
};
#endif
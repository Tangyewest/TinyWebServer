#include "httprequest.h"
using namespace std;


//这是一个无序集合，保存了一组常用的网页路径（如首页、注册、登录等）。
//作用：用于快速判断某个请求路径是否属于这些“默认页面”，比如判断用户请求的 URL 是否是这些页面之一。
const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/viedo",
    "/picture",
};

//这是一个无序映射表，将特定的 HTML 页面路径映射为一个整数标签（如注册页为 0，登录页为 1）。
//作用：用于根据页面路径快速获取对应的类型编号，方便后续逻辑处理（比如区分注册和登录页面）。
const unordered_map<string,int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1}, 
};

void HttpRequest::Init(){
    method_="";
    path_="";
    version_="";
    body_="";
    state_=REQUEST_LINE;
    headers_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const{
    if(headers_.find("connection")!= headers_.end())
    {
        return headers_.find("connection")->second == "keep-alive" &&
                version_ == "1.1";
    }
    return false;
}


bool HttpRequest::Parse(Buffer& buffer){
    const char CRLF[]= "\r\n";//行结束符标志(回车换行)
    if(buffer.ReadableBytes()<0)
    {
        return false;
    }
    //读取数据
    while(buffer.ReadableBytes()&& state_!=FINISH)
    {
        //从buff中的读指针开始到读指针结束，这块区域是未读取得数据并去处"\r\n"，返回有效数据得行末指针
        //找到 buffer 中一行数据的结尾（即 "\r\n" 的位置），用于按行解析 HTTP 请求。
        const char* lineEnd = search(buffer.peek(),buffer.BeginWriteConst(),CRLF,CRLF+2);
        //转化为string类型
        std::string line(buffer.peek(),lineEnd);
        switch(state_)
        {
            /*
            有限状态机，从请求行开始，每处理完后会自动转入到下一个状态    
            */
           case REQUEST_LINE:
                if(!ParseRequestLine(line))
                {
                    return false;
                }
                ParsePath_();//解析路径
                break;
            case HEADERS:
                ParseHeader(line);//解析请求头
                if(buffer.ReadableBytes()<=2)
                {
                    state_ = FINISH;
                }
                break;
            case BODY:
                ParseBody(line);//解析请求体
                break;
            default:
                break;
        }
        if(lineEnd == buffer.BeginWriteConst())
        {
            break;
        }//如果行末指针等于读指针，说明没有数据了
        buffer.retrieveUntil(lineEnd + 2);//把读过的数据取走（跳过回车换行）
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

//解析路径
void HttpRequest::ParsePath_(){
    if(path_ == "/")//如果请求路径是根目录
    {
        path_ = "/index.html";//默认首页
    }
    else
    {
        for(auto &item:DEFAULT_HTML)
        {
            if(path_ == item)
            {
                path_ += ".html";//如果请求路径是默认路径，添加后缀
                break;
            }     
    }
}

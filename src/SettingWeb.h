#include "cJSON\cJSON.h"
// 编码转换
std::wstring utf8to16(const char* src)
{
    std::vector<wchar_t> buffer;
    buffer.resize(MultiByteToWideChar(CP_UTF8, 0, src, -1, 0, 0));
    MultiByteToWideChar(CP_UTF8, 0, src, -1, &buffer[0], (int)buffer.size());
    return std::wstring(&buffer[0]);
}

// 编码转换
std::string utf16to8(const wchar_t* src)
{
    std::vector<char> buffer;
    buffer.resize(WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL));
    WideCharToMultiByte(CP_UTF8, 0, src, -1, &buffer[0], (int)buffer.size(), NULL, NULL);
    return std::string(&buffer[0]);
}

static void SendStaticFile(struct mg_connection *nc, const char *path)
{
    bool result = LoadFromResource("WEB", path, [&](const char *data, DWORD size)
    {
        mg_send_head(nc, 200, size, NULL);
        mg_send(nc, data, size);
    });

    if (!result)
    {
        const char reason[] = "Not Found";
        mg_send_head(nc, 404, sizeof(reason) - 1, NULL);
        mg_send(nc, reason, sizeof(reason) - 1);
    }
}

static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix)
{
    return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
}

// 获得系统语言
std::wstring GetDefaultLanguage()
{
    wchar_t language[MAX_PATH];
    if (!GetLocaleInfo(GetUserDefaultUILanguage(), LOCALE_SISO639LANGNAME, language, MAX_PATH))
    {
        return L"zh-CN";
    }
    wchar_t country[MAX_PATH];
    if (!GetLocaleInfo(GetUserDefaultUILanguage(), LOCALE_SISO3166CTRYNAME, country, MAX_PATH))
    {
        return std::wstring(language);
    }

    return std::wstring(language) + L"-" + country;
}

static void http_get(struct mg_connection *nc, struct http_message *hm)
{
    static const struct mg_str static_file = MG_MK_STR("/static/");
    if (mg_vcmp(&hm->uri, "/") == 0)
    {
        // 首页
        std::wstring Language = GetDefaultLanguage();
        if (_wcsicmp(Language.c_str(), L"zh-CN") == 0) {
            // 简体中文
            SendStaticFile(nc, "index.html");
        } else if (_wcsicmp(Language.c_str(), L"zh-TW") == 0) {
            // 台湾
            SendStaticFile(nc, "index-tw.html");
        } else if (_wcsicmp(Language.c_str(), L"zh-HK") == 0) {
            // 香港
            SendStaticFile(nc, "index-tw.html");
        } else {
            // 其它
            SendStaticFile(nc, "index.html");
        }
    }
    else if (has_prefix(&hm->uri, &static_file))
    {
        //静态文件
        char path[MAX_PATH];
        sprintf(path, "%.*s", (int)hm->uri.len - 1, hm->uri.p + 1);
        SendStaticFile(nc, path);
    }
    else
    {
        //404
        SendStaticFile(nc, "404.html");
    }
}

void ReadList(cJSON *root, const wchar_t *iniPath, const wchar_t *name)
{
    cJSON *list = cJSON_CreateArray();
    wchar_t additional_parameter[MAX_SIZE];
    GetPrivateProfileSectionW(name, additional_parameter, MAX_SIZE, iniPath);

    wchar_t *parameter_ptr = additional_parameter;
    while (parameter_ptr && *parameter_ptr)
    {
        cJSON_AddItemToArray(list, cJSON_CreateString(utf16to8(parameter_ptr).c_str()));
        parameter_ptr += wcslen(parameter_ptr) + 1;
    }
    cJSON_AddItemToObject(root, utf16to8(name).c_str(), list);
}
void ReadValue(cJSON *node, const wchar_t *iniPath, const wchar_t *section, const wchar_t *name)
{
    wchar_t value[256];
    GetPrivateProfileStringW(section, name, L"", value, MAX_PATH, iniPath);
    cJSON_AddItemToObject(node, utf16to8(name).c_str(), cJSON_CreateString(utf16to8(value).c_str()));
}

static void http_post(struct mg_connection *nc, struct http_message *hm)
{
    const wchar_t *iniPath = (const wchar_t *)nc->mgr->user_data;

    if (mg_vcmp(&hm->uri, "/get_setting") == 0) {
        cJSON *root = cJSON_CreateObject();
        ReadList(root, iniPath, L"追加参数");
        ReadList(root, iniPath, L"启动时运行");
        ReadList(root, iniPath, L"关闭时运行");
        ReadList(root, iniPath, L"鼠标手势");

        cJSON *node = cJSON_CreateObject();
        ReadValue(node, iniPath, L"基本设置", L"数据目录");
        ReadValue(node, iniPath, L"基本设置", L"老板键");
        ReadValue(node, iniPath, L"基本设置", L"新标签页面");
        ReadValue(node, iniPath, L"基本设置", L"移除更新错误");
        ReadValue(node, iniPath, L"基本设置", L"自动结束运行程序");
        ReadValue(node, iniPath, L"基本设置", L"恢复NPAPI");
        ReadValue(node, iniPath, L"基本设置", L"便携化");
        cJSON_AddItemToObject(root, utf16to8(L"基本设置").c_str(), node);

        node = cJSON_CreateObject();
        ReadValue(node, iniPath, L"界面增强", L"双击关闭标签页");
        ReadValue(node, iniPath, L"界面增强", L"右键关闭标签页");
        ReadValue(node, iniPath, L"界面增强", L"保留最后标签");
        ReadValue(node, iniPath, L"界面增强", L"悬停快速标签切换");
        ReadValue(node, iniPath, L"界面增强", L"右键快速标签切换");
        ReadValue(node, iniPath, L"界面增强", L"新标签打开书签");
        ReadValue(node, iniPath, L"界面增强", L"新标签打开网址");
        ReadValue(node, iniPath, L"界面增强", L"新标签页不生效");
        ReadValue(node, iniPath, L"界面增强", L"前台打开新标签");
        cJSON_AddItemToObject(root, utf16to8(L"界面增强").c_str(), node);

        node = cJSON_CreateObject();
        ReadValue(node, iniPath, L"鼠标手势", L"启用");
        ReadValue(node, iniPath, L"鼠标手势", L"轨迹");
        ReadValue(node, iniPath, L"鼠标手势", L"动作");
        cJSON_AddItemToObject(root, utf16to8(L"鼠标手势开关").c_str(), node);

        char *str = cJSON_PrintUnformatted(root);
        int len = strlen(str);
        cJSON_Delete(root);

        mg_send_head(nc, 200, len, "Content-Type: text/xml; charset=utf-8");
        mg_send(nc, str, len);
        free(str);
    } else if (mg_vcmp(&hm->uri, "/set_setting") == 0) {
        char section[200];
        char name[200];
        char value[200];
        mg_get_http_var(&hm->body, "section", section, sizeof(section));
        mg_get_http_var(&hm->body, "name", name, sizeof(name));
        mg_get_http_var(&hm->body, "value", value, sizeof(value));
        //DebugLog(L"set_setting %s %s=%s", utf8to16(section).c_str(), utf8to16(name).c_str(), utf8to16(value).c_str());

        ::WritePrivateProfileString(utf8to16(section).c_str(), utf8to16(name).c_str(), utf8to16(value).c_str(), iniPath);

        mg_send_head(nc, 200, 2, "Content-Type: text/xml; charset=utf-8");
        mg_send(nc, "{}", 2);
    /*
    } else if (mg_vcmp(&hm->uri, "/set_section") == 0) {
        char section[200];
        char value[200];
        mg_get_http_var(&hm->body, "section", section, sizeof(section));
        mg_get_http_var(&hm->body, "value", value, sizeof(value));
        //DebugLog(L"set_setting %s %s=%s", utf8to16(section).c_str(), utf8to16(name).c_str(), utf8to16(value).c_str());

        std::wstring wvalue = utf8to16(value);
        wvalue += L'\0';

        ::WritePrivateProfileSection(utf8to16(section).c_str(), NULL, iniPath);
        ::WritePrivateProfileSection(utf8to16(section).c_str(), wvalue.c_str(), iniPath);

        mg_send_head(nc, 200, 2, "Content-Type: text/xml; charset=utf-8");
        mg_send(nc, "{}", 2);
     */
    } else if (mg_vcmp(&hm->uri, "/del_setting") == 0) {
        char section[200];
        char name[200];
        mg_get_http_var(&hm->body, "section", section, sizeof(section));
        mg_get_http_var(&hm->body, "name", name, sizeof(name));
        //DebugLog(L"set_setting %s %s=%s", utf8to16(section).c_str(), utf8to16(name).c_str());

        ::WritePrivateProfileString(utf8to16(section).c_str(), utf8to16(name).c_str(), NULL, iniPath);

        mg_send_head(nc, 200, 2, "Content-Type: text/xml; charset=utf-8");
        mg_send(nc, "{}", 2);
    }else{
        //404
        SendStaticFile(nc, "404.html");
    }
}

static void http_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_HTTP_REQUEST:
    {
        struct http_message *hm = (struct http_message *) ev_data;
        //DebugLog(L"%.*S %.*S %.*S", (int)hm->method.len, hm->method.p, (int)hm->uri.len, hm->uri.p, (int)hm->body.len, hm->body.p);

        if (mg_vcmp(&hm->method, "GET") == 0) {
            http_get(nc, hm);
        }
        else if (mg_vcmp(&hm->method, "POST") == 0) {
            http_post(nc, hm);
        }
        else {
            //404
            SendStaticFile(nc, "404.html");
        }
        nc->flags |= MG_F_SEND_AND_CLOSE;
    }
    break;
    default:
        break;
    }
}

#pragma data_seg(".SHARED")
int http_port = 0;
#pragma data_seg()
#pragma comment(linker, "/section:.SHARED,RWS")

void WebThread(const std::wstring iniPath, const HANDLE ready_event)
{
    struct mg_mgr mgr;
    struct mg_connection *nc;

    // 延迟一点，否则死锁
    Sleep(100);
    mg_mgr_init(&mgr, (void *)iniPath.c_str());

    for (http_port = 80; http_port < 65535; http_port++)
    {
        char http_address[1024];
        wsprintfA(http_address, "127.0.0.1:%d", http_port);
        if (nc = mg_bind(&mgr, http_address, http_handler))
        {
            break;
        }
    }

    SetEvent(ready_event);
    if (nc)
    {
        mg_set_protocol_http_websocket(nc);
        for (;;) {
            mg_mgr_poll(&mgr, 100);
        }
    }
    else
    {
        http_port = 80;
        DebugLog(L"WebThread failed");
    }
    mg_mgr_free(&mgr);
}

void SettingWeb(const wchar_t *iniPath)
{
    if (GetPrivateProfileInt(L"基本设置", L"人家不要WEB啦", 0, iniPath) == 1)
    {
        return;
    }
    HANDLE ready_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    std::thread th(WebThread, iniPath, ready_event);
    th.detach();

    ::WaitForSingleObject(ready_event, INFINITE);
}
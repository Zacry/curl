// Copyright (c) 2013, Tencent Inc. All Rights Reserved.
// Author: Lei Wang (pecywang@tencent.com)
// Date: 2013-05-23

#ifndef SODEPEND_HTTP_SERVICE_H_
#define SODEPEND_HTTP_SERVICE_H_
#pragma once

#include <assert.h>
#include <string.h>
#include <string>
#include <map>
#include <stdarg.h>
#include "curl64/curl.h"

namespace oms {
namespace comm {

typedef std::map<std::string, std::string> HttpParams;

struct BuffV
{
    BuffV() : m_used_len(0), m_total_len(0)
    {
        // Ĭ�Ϸ���1K
        m_total_len = 256 * 1024 * 1024;
        m_buf = new char[m_total_len];
        memset(m_buf, 0, sizeof(m_buf));
        assert(NULL != m_buf);
    }

    ~BuffV()
    {
        if(m_buf) {
            delete m_buf;
            m_buf = NULL;
        }
    }

    // ���buf�ĳ����Ƿ����need_len
    // ���С�����Զ���չ
    bool CheckBuffer(size_t need_len)
    {
        // ���֧��1M
        if(need_len > 1024 * 1024) {
            return false;
        }

        if(m_total_len > need_len) {
            return true;
        }

        m_total_len = need_len + 256; // ������һЩ
        char* new_buf = new char[m_total_len];
        assert(NULL != new_buf);
        memset(new_buf, 0, m_total_len);

        if(m_used_len) {
            memcpy(new_buf, m_buf, m_used_len);
        }

        delete m_buf;
        m_buf = new_buf;
        return true;
    }

    void SetBuffer(const void* ptr, size_t len)
    {
        memcpy(m_buf + m_used_len, ptr, len);
        m_used_len += len;
    }

    const char* Ptr() const
    {
        return m_buf;
    }

    size_t Size() const
    {
        return m_used_len;
    }

    size_t Capacity() const
    {
        return m_total_len;
    }

private:
    char* m_buf;

    // ��ǰʹ�ó���
    size_t m_used_len;

    // ��ǰ�ܳ���
    size_t m_total_len;

};

typedef unsigned char BYTE;
inline BYTE toHex(const BYTE &x)
{
    return x > 9 ? x - 10 + 'a': x + '0';
}
inline BYTE fromHex(const BYTE &x)
{
    return isdigit(x) ? x-'0' : x-'a' + 10;
}
inline std::string HttpURLEncoding(const std::string &sIn)
{
    std::string sOut;
    for( size_t ix = 0; ix < sIn.size(); ix++ )
    {
        BYTE buf[4];
        memset( buf, 0, 4 );
        if( isalnum( (BYTE)sIn[ix] ) )
        {
            buf[0] = sIn[ix];
        }
        else
        {
            buf[0] = '%';
            buf[1] = toHex( (BYTE)sIn[ix] >> 4 );
            buf[2] = toHex( (BYTE)sIn[ix] % 16);
        }
        sOut += (char *)buf;
    }
    return sOut;
};

// ���յ���ݵĻص�����
// @param curl���յ���������ڻ�����
// @param [in] size ��ݳ���
// @param [in] nmemb ���Ƭ����
// @param [in/out] �û��Զ���ָ��
// @return ��ȡ����ݳ���
size_t CurlCallback(void* ptr, size_t size, size_t nmemb, void* userp)
{
    size_t read_bytes = size * nmemb;
    BuffV* buf = static_cast<BuffV*>(userp);

    if(!buf->CheckBuffer(buf->Size() + read_bytes)) {
        printf("Can't get enough memory!\n");
        return read_bytes;
    }

    buf->SetBuffer(ptr, read_bytes);

    //printf("read_bytes:%lu\n", read_bytes);
    return read_bytes;
}

class CurlHttp
{
public:
    virtual ~CurlHttp()
    {
        curl_easy_cleanup(m_curl);
    };
    
    static CurlHttp* GetInstance()
    {
        static CurlHttp curl_http;
        return &curl_http;
    };

    // ��������ĸ��Ӳ���
    // ��ʽ:key=value&key=value
    // POST���󴫲�����ô˺���
    void SetHttpParams(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        vsnprintf(m_params, sizeof(m_params), format, ap);
        va_end(ap);
    };

    void ClearHeader()
    {
        if (m_pHeaders)
		{
			curl_slist_free_all(m_pHeaders);
			m_pHeaders = NULL;
		}
    };

    void ClearParam()
    {
        memset(m_params, 0, sizeof(m_params));
    };
    // ���ó�ʱ
    void SetTimeout(long timeout)
    {
        // 设置超时
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, timeout);
    };
    void SetHeader(const char* header)
    {
        m_pHeaders=curl_slist_append(m_pHeaders, header);
    };
    void SetProxy(const char* proxyIp)
    {
        // 设置代理IP
        curl_easy_setopt(m_curl, CURLOPT_HTTPPROXYTUNNEL, true);
        curl_easy_setopt(m_curl, CURLOPT_PROXY, proxyIp);
        //curl_easy_setopt(m_curl, CURLOPT_PROXYUSERPWD, 'user:password');
    };

    // ����GET��POST����
    // @param: buf ����HTTP���󵽵�body����
    int HttpRequest(const std::string& url, BuffV* buf, bool is_get = true)
    {
        if(url.empty()) {
            return -100000;
        }

        CURLcode iCurlRet;

        std::string addr = url;
        if(is_get) {
            curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
            AppendGetParam(&addr);
        } else {
            curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, m_params);
            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, strlen(m_params));
            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_pHeaders);
        }

        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, buf);

        //printf("addr:%s\n", addr.c_str());
        curl_easy_setopt(m_curl, CURLOPT_URL, addr.c_str());

        iCurlRet = curl_easy_perform(m_curl);

        return iCurlRet;
    };

private:
    CurlHttp()
    {
        m_curl = curl_easy_init();
        assert(NULL != m_curl);

        SetTimeout(5L);

        curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1l);
        curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1l);
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CurlCallback);


        memset(m_params, 0, sizeof(m_params));
    };
    CurlHttp(const CurlHttp&);
    CurlHttp& operator = (const CurlHttp&);
    
    void AppendGetParam(std::string* addr)
    {
        if(m_params[0] == '\0') {
            return;
        }

        addr->append("?");
        addr->append(m_params);
    };

private:
    CURL* m_curl;
    struct curl_slist *m_pHeaders;
    char m_params[2048*10];
};

} // so
} // oms

#endif // SODEPEND_HTTP_SERVICE_H_

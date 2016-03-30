#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include "the3/curl_http.h"
#include "the3/jsoncpp/json.h"

using namespace std;
using namespace oms::comm;

string g_strJdUrl = "http://ioms.360buy.com:10127/oom/service.ashx";

int main(int argc,char* argv[])
{
	if(argc != 2)
	{
		cout << "[ERROR] usage: " << argv[0] << " sysno" << endl;
		return 1;
	}

	int dwRet;
	std::stringstream oss;
	BuffV buf;

	oss << "Token=C99B094B08A87D68571B8F35572392A3" << "&orderId=" << argv[1];

	CurlHttp::GetInstance()->SetHttpParams(oss.str().c_str());
	CurlHttp::GetInstance()->SetHeader("Content-Type: application/json");
	CurlHttp::GetInstance()->SetHeader("charset: utf-8");

	dwRet = CurlHttp::GetInstance()->HttpRequest(g_strJdUrl.c_str(), &buf);
	if(dwRet != 0)
	{
		cout << "Curl error,dwRet[" << dwRet << "],sysno[" << argv[1] << "]" << endl;
	}


	Json::Value jResp;
	Json::Reader jReader;
	jReader.parse(buf.Ptr(), jResp);

	if(jResp["Code"].isInt() && jResp["Code"].asInt() == 1)
	{
		if(jResp["dataStr"].isNull())
		{
				cout << "dataStr value doesn't exist for sysno" << argv[1] << endl;
				return 1;
		}

		if(jResp["dataStr"].isString())
		{
			string strDeal = jResp["dataStr"].asString();
			cout << "dataStr: " << strDeal << endl;
		}
	}

	return 0;
}

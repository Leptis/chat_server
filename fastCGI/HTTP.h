/* 
 * File:   HTTP.h
 * Author: Novikov Vladislav
 *
 * Created on 26 Ноябрь 2014 г., 17:15
 */

#include <iostream>

#include <string.h>

#include <sstream>

#include <vector>

#include <stdlib.h>

//#include "sdb.h"

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
extern char ** environ;
#endif
#include "fcgio.h"
#include "fcgi_config.h"  // HAVE_IOSTREAM_WITHASSIGN_STREAMBUF
//#include "fcgi_stdio.h"
#include "fcgiapp.h"
#include "unistd.h"

using namespace std;

#ifndef HTTP_H
#define	HTTP_H

#define USE_CIN 0
#define USE_COUT 0

struct _getItemData
{
    string paramName;
    string value;
};

struct _envItemData
{
    string paramName;
    string value;
};

struct _cookieItemData
{
    string paramName;
    string value;
};

struct _userFile
{
    string paraName;
    string fileName;
    string type;
    string tmpName;
    size_t size;
    int error;
};

typedef vector<_userFile> filesVector;
typedef vector<string> getVector;
typedef getVector postVector;

struct _postItemData
{
    string paramName;
    string data;
};

struct _headerPostItemData
{
    string paramName;
    string value;
};

#define BUFF_SIZE 256

#define TMPFOLDER tmp/

enum file_upload_error{
    UPLOAD_ERR_OK = 0,
    UPLOAD_ERR_SIZE,
    UPLOAD_ERR_NO_FILE
};

enum http_status_code{
    // 1xx: Informational
    status_100_Continue = 100,
    status_101_Switching_Protocols,
    status_102_Processing,
    status_105_Name_Not_Resolved = 105,
    // 2xx: Success
    status_200_OK = 200,
    status_201_Created,
    status_202_Accepted,
    status_203_Non_Authoritative_Information,
    status_204_No_Content,
    status_205_Reset_Content,
    status_206_Partial_Content,
    status_207_Multi_Status,
    status_226_IM_Used = 226,
    // 3xx: Redirection
    status_300_Multiple_Choices = 300,
    status_301_Moved_Permanently,
    status_302_Moved_Temporarily,
    status_303_See_Other,
    status_304_Not_Modified,
    status_305_Use_Proxy,
    status_306_Unused,
    status_307_Temporary_Redirect,
    // 4xx: Client Error
    status_400_ = 400,
    status_401_,
    status_402_,
    status_403_,
    status_404_,
    status_405_,
    status_406_,
    status_407_,
    status_408_,
    status_409_,
    status_410_,
    status_411_,
    status_412_,
    status_413_,
    status_414_,
    status_415_,
    status_416_,
    status_417_,
    status_418_,
    status_422_ = 422,
    status_423_,
    status_424_,
    status_425_,
    status_426_,
    status_428_ = 428,
    status_429_,
    status_431_ = 431,
    status_434_ = 434,
    status_449_ = 449,
    status_451_ = 451,
    status_456_ = 456,
    status_499_ = 499,
    // 5xx: Server Error
    status_500_ = 500,
    status_501_,
    status_502_,
    status_503_,
    status_504_,
    status_505_,
    status_506_,
    status_507_,
    status_508_,
    status_509_,
    status_510_,
    status_511_
};

class HTTP {
public:
    static const unsigned long STDIN_MAX = 1024*1024*10;
    
    HTTP(unsigned int new_sock_id);
    HTTP(const HTTP& orig);
    ~HTTP();
    
    string NumberToString (int);
    
    int acceptConnection();
    void freeConnection();

    void attachStreams();
    
    FCGX_Stream* getCin();
    FCGX_Stream* getCout();
    FCGX_Stream* getCerr();
    
    void err(string);
    
    string getEnv(string paramName); //Получение переменных окружения 
    string getEnv(int id);
    void printAllEnv();
    
    void loadEnvItems();
    void loadPostData();

    string findValueByName(string name, vector<_headerPostItemData> vec);
    void setHeader(string paramName, string paramValue);
    void setHTTPStatus(int);
    void setHTTPStatus(int code, string text);
    void redirect(string url);
    void redirectPermanent(string url);
    inline void ok() {setHTTPStatus(status_200_OK);}
    void setBody(string value);
    void collectData();
    
    string get(string paramName);
    getVector getArray(string paramName);
    string post(string paramName);
    postVector postArray(string paramName);
    string getHeader(string paramName);
    
    string getCookie(string paramName);
    void setCookie(string name, string value, int expire = 0, string path = "", string domain = "");
    
    string getSession(string paramName);
    void setSession(string paramName, string paramValue);
    
    struct _userFile files(string name);
    filesVector filesArray(string name);

    bool isInProcess();

    void print_Files();

private:
    streambuf * cin_streambuf;
    streambuf * cout_streambuf;
    streambuf * cerr_streambuf;

    fcgi_istream cin_stream;
    fcgi_ostream cout_stream;
    fcgi_ostream cerr_stream;


    FCGX_Request request;
    
    vector<_getItemData> _GET;
    vector<_postItemData> _POST;
    vector<_envItemData> _Env;
    vector<_cookieItemData> _Cookie;
    
    vector<_userFile> _Files;
    
    string _header_buff;
    string _body_buff;
    
    int buff_index;
    char buff[BUFF_SIZE+1];
    char * c_len_str;
    char * c_type_str;
    string boundary;
    string _boundary;
    int blockCount;
    unsigned long c_len;
    
    unsigned char state;
    size_t index;
    size_t boundary_length;
    
    void loadGetItems(string);
    void loadCookie(string);
    
    void addEnvItem(const char*);
    void printEnvToLog();
    void printGetToLog();
    
    char getCharFromStdin();
    
    void parseMultiPart();
    
    void addPostItem(string paramName, string value);
    
    string getpostParamNameValue(string str);
    string getpostFileNameValue(string str);
    
    void newFile(string paramName, string filename, string type, string tmpName, size_t size);
    
    string generateTmpName();
    
    size_t fileSize(string fileName);
    string getResponseCode(int iResponseCode);

    ///////// Leptis
    bool processing = false;

    unsigned int sock_id;
};

#endif	/* HTTP_H */


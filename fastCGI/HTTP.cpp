/* 
 * File:   HTTP.cpp
 * Author: Novikov Vladislav
 * 
 * Created on 26 Ноябрь 2014 г., 17:15
 */

#include "HTTP.h"

HTTP::HTTP(unsigned int new_sock_id)
{
    sock_id = new_sock_id;
    FCGX_InitRequest(&request, sock_id, 0);
}

HTTP::HTTP(const HTTP& orig) 
{
    
}

HTTP::~HTTP() 
{
    FCGX_Free(&request, 1);
}

string HTTP::NumberToString (int Number)
{
    ostringstream ss;
    ss << Number;
    return ss.str();
}

void HTTP::setHeader(string paramName, string paramValue)
{
    _header_buff+=paramName+": "+paramValue+"\r\n";
}

void HTTP::setBody(string value) 
{
    this->_body_buff+=value;
}

void HTTP::collectData() {
    if (USE_COUT) {
        cout << this->_header_buff << "\r\n" << this->_body_buff;

    }else{
        cout_stream << this->_header_buff << "\r\n" << this->_body_buff;
    }
}

void HTTP::setCookie(string name, string value, int expire, string path, string domain)
{
    string tmp;
    tmp+=name;
    tmp+='=';
    tmp+=value;
    if(expire)
    {
        // время жизни 
    }
    if(!path.empty())
    {
        tmp+="; path="+path;
    }
    if(!domain.empty())
    {
        tmp+="; domain="+domain;
    }
    setHeader("Set-Cookie", tmp);
}

void HTTP::loadEnvItems() 
{
    char* const * envp = request.envp;
    this->_header_buff.clear();
    this->_body_buff.clear();
    if(!this->_Env.empty()) this->_Env.clear();
    for ( ; *envp; ++envp)
    {
        addEnvItem(*envp);
    }
    //printEnvToLog();
}

void HTTP::loadGetItems(string str) 
{
    //cerr << "Loading get items" << endl;
    if(!this->_GET.empty()) this->_GET.clear();
    const char* _str = str.c_str();
    const char* start = _str, *end = _str, *cur = _str, *tmp = NULL;
    if(str.empty()) 
    {
        //cerr << "Get str is empty" << endl;
        return;
    }
    do
    {
        if(*cur=='&'||*cur=='\0')
        {
            end = cur;
            tmp = start;
            while(*tmp!='=' && tmp!=cur) tmp++;
            string paramName(start,tmp-start);
            this->_GET.push_back(_getItemData());
            this->_GET.back().paramName = paramName;
            this->_GET.back().value = (tmp==cur?(string)"":string(tmp+1, cur-tmp-1));
            //cerr << "Insert into vector - " << this->_GET.back().paramName << " = " << this->_GET.back().value << endl;
            start = end + 1;
        }
    }while(*(cur++));
}

void HTTP::loadPostData() 
{
    int ct = 0;
    this->c_len_str = FCGX_GetParam("CONTENT_LENGTH", this->request.envp);
    this->c_type_str = FCGX_GetParam("CONTENT_TYPE", this->request.envp);
    this->c_len = STDIN_MAX;
    this->blockCount = 0;
    this->buff_index = BUFF_SIZE;
    this->buff[BUFF_SIZE] = '\0';
    this->_POST.clear();
    this->_Files.clear();

    if (c_len_str)
    {

        this->c_len = strtol(c_len_str, &c_len_str, 10);
        //cerr << "CONTENT_LENGTH - " << this->clen << endl;
        if (*c_len_str)
        {
            /*
             * cerr << "can't parse \"CONTENT_LENGTH="
                 << FCGX_GetParam("CONTENT_LENGTH", this->request.envp)
                 << "\"\n";
            */
            return;
        }
        if (this->c_len > STDIN_MAX) this->c_len = STDIN_MAX;
        //cerr << "CONTENT_TYPE = " << FCGX_GetParam("CONTENT_TYPE", this->request.envp) << endl;
        if(!strcmp(c_type_str, "text/plain") || !strcmp(c_type_str, "application/x-www-form-urlencoded")) ct = 1;
        else 
        {
            char*tmp = c_type_str;
            char*end = c_type_str;
            while(*end++) ;
            while(*tmp != ';' && *tmp) tmp++;
            if(!*tmp)
            {
                //Неизвестный CONTENT_TYPE
                // Надо бы отправить exception...
                // А пока
                cerr << "Can't parse CONTENT_TYPE" << endl;
                return;
            }
            string cty(c_type_str, tmp-c_type_str);
            string bound(tmp+2+9, end-(tmp+2)-10);
            this->_boundary = bound;
            this->boundary = "--"+this->_boundary;
            if(strcmp(cty.c_str(), "multipart/form-data"))
            {
                //Неизвестный CONTENT_TYPE
                // Надо бы отправить exception...
                // А пока
                //cerr << "Unknown CONTENT_TYPE" << endl;
                return;
            }
            //cerr << "ctype = " << cty << endl;
            //cerr << "boundary = " << this->_boundary << endl;
            //cout << "<br>" << this->boundary << "<br>";
        }

        //cerr << "====\nReading blocks from stdin" << endl;
        char ch = 0;
        int state = 0;
        string paramName;
        string value;
        bool is_last = false;
        if(ct == 1)
        {

            // text/plain and application/x-www-form-urlencoded
            //cerr << "parsing - text/plain and application/x-www-form-urlencoded" << endl;
            for(int i = 0; i < this->c_len; i++)
            {

                //printf(ch);
                is_last = (i == (this->c_len - 1));
                ch = this->getCharFromStdin();
                //cout << ch;

                //cout << ch;
                if(ch == '=')
                {
                    //cerr << "POST - " << paramName;
                    state = 1;
                    continue;
                }
                if(ch == '&' || ch == '\r')
                {
                    //cerr << " = " << value << endl;
                    state = 0;
                    addPostItem(paramName, value);
                    paramName.clear();
                    value.clear();
                    continue;
                }
                if(ch == '\n'){
                    continue;
                }
                if(is_last)
                {
                    value+=ch;
                    //cerr << " = " << value << endl;
                    state = 0;
                    addPostItem(paramName, value);
                    paramName.clear();
                    value.clear();
                    continue;
                }
                switch(state)
                {
                    case 0:
                        paramName+=ch;
                        break;
                    case 1:
                        value+=ch;
                        break;
                }
            }
        }
        else
        {
            //cout << 100;
            // multipart/form-data
            char ch = 0;
            //cerr << "parsing - multipart/form-data" << endl;
            this->boundary_length = this->boundary.size();
            parseMultiPart();
        }
        //cerr << "Stop reading\n====" << endl;
    }
    else
    {
        //cerr << "CONTENT_LENGTH is empty" << endl;
        this->buff[0] = 0;
        c_len = 0;
    }
}

void HTTP::addEnvItem(const char*str) 
{
    const char* tmp = str;
    while(*tmp!='=') tmp++;
    string paramName(str,tmp-str);
    string value((*(tmp+1)!='\0')?tmp+1:""); // magic
    //cerr << "Found " << paramName << " " << value << endl;
    if(paramName == "QUERY_STRING") loadGetItems(value);
    else if(paramName == "HTTP_COOKIE") loadCookie(value);
    else
    {
        this->_Env.push_back(_envItemData());
        this->_Env.back().paramName = paramName;
        this->_Env.back().value = value;
    }
}

int HTTP::acceptConnection()
{
    if(FCGX_Accept_r(&request) == 0) {
        processing = true;
        return 1;
    }
    else return 0;
}

void HTTP::freeConnection() 
{
    FCGX_Finish_r(&request);
    processing = false;
}

string HTTP::getEnv(string paramName)
{
    for(int i = 0; i < this->_Env.size(); i++)
        if(this->_Env[i].paramName == paramName)
            return this->_Env[i].value;
    return "";
}

string HTTP::getEnv(int id)
{
    return (this->_Env.at(id)).value;
}

FCGX_Stream* HTTP::getCin() 
{
    return this->request.in;
}

FCGX_Stream* HTTP::getCout() 
{
    return this->request.out;
}

FCGX_Stream* HTTP::getCerr() 
{
    return this->request.err;
}

string HTTP::get(string paramName) 
{
    for(int i = 0; i<this->_GET.size();i++) 
        if(this->_GET[i].paramName == paramName) 
            //return (this->_GET[i].value.empty())?NULL:this->_GET[i].value;
            return this->_GET[i].value;
    return (string)""; 
}

string HTTP::post(string paramName) 
{
    for(int i = 0; i < this->_POST.size(); i++)
    {
        if(this->_POST[i].paramName == paramName) return this->_POST[i].data;
    }
    return string();
}

void HTTP::err(string str) 
{
    /*
    fcgi_streambuf cin_fcgi_streambuf(this->getCin());
    fcgi_streambuf cout_fcgi_streambuf(this->getCout());
    fcgi_streambuf cerr_fcgi_streambuf(this->getCerr());
    
#if HAVE_IOSTREAM_WITHASSIGN_STREAMBUF
    cin  = &cin_fcgi_streambuf;
    cout = &cout_fcgi_streambuf;
    cerr = &cerr_fcgi_streambuf;
#else
    cin.rdbuf(&cin_fcgi_streambuf);
    cout.rdbuf(&cout_fcgi_streambuf);
    cerr.rdbuf(&cerr_fcgi_streambuf);
#endif*/
    cerr << endl << str << endl;
}

void HTTP::printEnvToLog() 
{
    err("====");
    err("ENV vector");
    err("====");
    for(int i = 0; i<this->_Env.size();i++)
    {
        cerr << _Env[i].paramName << " = " << _Env[i].value << endl;
    }
    err("====");
}

void HTTP::printGetToLog() 
{
    err("====");
    err("GET vector");
    err("====");
    for(int i = 0; i<this->_GET.size();i++)
    {
        cerr << _GET[i].paramName << " = " << _GET[i].value << endl;
    }
    err("====");
}

char HTTP::getCharFromStdin() 
{
    if(this->buff_index == BUFF_SIZE)
    {
        if(this->blockCount*BUFF_SIZE<this->c_len)
        {
            for(int i = 0; i < BUFF_SIZE; i++) {this->buff[i] = -1;}
#if USE_CIN
            if (this->clen - (this->blockCount * BUFF_SIZE) < BUFF_SIZE)
                cin.read(this->buff, this->clen - (this->blockCount * BUFF_SIZE));
            else
                cin.read(this->buff, BUFF_SIZE);
#else
            if (this->c_len - (this->blockCount * BUFF_SIZE) < BUFF_SIZE)
                cin_stream.read(this->buff, this->c_len - (this->blockCount * BUFF_SIZE));
            else
                cin_stream.read(this->buff, BUFF_SIZE);
#endif
            printf("buf2");
            this->blockCount++;
            this->buff_index = 0;
            return this->buff[this->buff_index++];
        }
    }
    else
        return this->buff[buff_index++];
}

struct _userFile HTTP::files(string name) 
{
    cerr << "files - " << name << endl;
    for(int i = 0; i < this->_Files.size(); i++)
        if(this->_Files[i].paraName == name) return this->_Files[i];
    cerr << "don't find" << endl; 
    struct _userFile empty;
    empty.error = UPLOAD_ERR_NO_FILE; // файл не загружен
    return empty;
}

void HTTP::parseMultiPart() 
{
    enum state {
        s_uninitialized = 1,
        s_start,
        s_start_boundary,
        s_header_field_start,
        s_header_field,
        s_headers_almost_done,
        s_header_value_start,
        s_header_value,
        s_header_value_almost_done,
        s_part_data_start,
        s_part_data,
        s_part_data_almost_boundary,
        s_part_data_boundary,
        s_part_data_almost_end,
        s_part_data_end,
        s_part_data_final_hyphen,
        s_end
    };

    #define LF 10
    #define CR 13

    int ch = 0;
    int cl = 0;
    
    int is_last = 0;
    
    bool file = false;
    
    string buff;
    string headerName;
    string headerValue;
    string dataValue;
    vector<_headerPostItemData> headers;
    
    string paramName;
    string fileName;
    string type;
    string tmpName;
    size_t size;
    
    FILE*f = NULL;
    
    this->state = s_start;
    
    for(int i = 0; i < this->c_len; i++)
    {
        is_last = (i == (this->c_len - 1));
        ch = this->getCharFromStdin();
        //if(false)
        switch(this->state)
        {
            case s_start:
                //cerr << "s_start" << endl;
                this->index = 0;
                this->state = s_start_boundary;
                srand(time(0));
                
            case s_start_boundary:
                //cerr << "s_start_boundary" << endl;
                if(this->index == this->boundary_length)
                {
                    if(ch != CR) return;
                    this->index++;
                    break;
                }
                else 
                    if(this->index == (this->boundary_length + 1))
                    {
                        if(ch != LF) return;
                        this->index = 0;
                        this->state = s_header_field_start;
                        break;
                    }
                if(ch != this->boundary.c_str()[this->index]) return;
                this->index++;
                break;
                
            case s_header_field_start:
                //cerr << "s_header_field_start" << endl;
                headerName.clear();
                headerValue.clear();
                this->state = s_header_field;
                
            case s_header_field:
                //cerr << "s_header_field" << endl;
                if (ch == CR)
                {
                    //все кароч
                    this->state = s_headers_almost_done;
                    break;
                }
                if (ch == '-')
                {
                    headerName+=ch;
                    break;
                }

                if (ch == ':') 
                {
                    //собрано имя параметра
                    this->state = s_header_value_start;
                    break;
                }
                
                cl = tolower(ch);
                if (cl < 'a' || cl > 'z') return;
                if (is_last)
                {
                    //кароч кончилось
                }
                headerName+=ch; //если это простой символ, то пилим его в имя параметра
                break;
                
            case s_headers_almost_done:
                //cerr << "s_headers_almost_done" << endl;
                if (ch != LF) return;
                this->state = s_part_data_start;
                break;
                
            case s_header_value_start:
                //cerr << "s_header_value_start" << endl;
                if (ch == ' ') break;
                this->state = s_header_value;
                
            case s_header_value:
                //cerr << "s_header_value" << endl;
                if (ch == CR) this->state = s_header_value_almost_done;
                if (is_last)
                {
                    //кароч кончилось
                }
                headerValue+=ch;
                break;
                
            case s_header_value_almost_done:
                //cerr << "s_header_value_almost_done" << endl;
                //параметр собран полностью
                //тут его надо добавить в вектор
                //cout << headerName << " = " << headerValue << "<br>";
                this->getpostFileNameValue(headerValue);
                this->getpostParamNameValue(headerValue);
                headers.push_back(_headerPostItemData());
                headers.back().paramName = headerName;
                headers.back().value = headerValue;
                this->state = s_header_field_start;
                break;
                
            case s_part_data_start:
                //cerr << "s_part_data_start" << endl;
                //headers собраны все
                //тут можно с ними что-нибудь сделать 
                //а можно и не делать 
                if(headers[headers.size()-1].paramName == "ContentType" && !getpostFileNameValue(headers[headers.size()-2].value).empty())
                {
                    fileName = getpostFileNameValue(headers[headers.size()-2].value);
                    paramName = getpostParamNameValue(headers[headers.size()-2].value);
                    file = true;
                    string filename;
                    filename+="/tmp/";
                    filename+=generateTmpName();
                    tmpName = filename;
                    //cerr << filename << endl;
                    f = fopen(filename.c_str(),"wb");
                    if(!f) 
                    {
                        //cerr << "can't open file " << filename << endl;
                        return;
                    }
                    type = headers[headers.size()-1].value;
                }
                dataValue.clear();
                buff.clear();
                this->state = s_part_data;
                
            case s_part_data:
                //cerr << "s_part_data" << endl;
                if(ch == CR)
                {
                    //возможно начинается boundary
                    buff+=CR;
                    this->state = s_part_data_almost_boundary;
                    break;
                }
                if (is_last)
                {
                    //кароч кончилось
                }
                if(file) fputc(ch, f);
                else
                dataValue+=ch;
                break;
            case s_part_data_almost_boundary:
                //cerr << "s_part_data_almost_boundary" << endl;
                if(ch == LF)
                {
                    //ну скорее всго это baundary
                    //проверяем
                    buff+=LF;
                    this->index = 0;
                    this->state = s_part_data_boundary;
                    break;
                }
                if(file) 
                {
                    fwrite((void*)buff.c_str(), sizeof(char), buff.size(), f);
                    fputc(ch, f);
                }
                else
                dataValue+=buff;
                buff.clear();
                this->state = s_part_data;
                break;
            case s_part_data_boundary:
                //cerr << "s_part_data_boundary" << endl;
                //проверяем boundary на правильность 
                if(this->boundary.c_str()[this->index] != ch) 
                {
                    //мы ошиблись
                    this->state = s_part_data;
                    if(file) 
                    {
                        fwrite((void*)buff.c_str(), sizeof(char), buff.size(), f);
                        fputc(ch, f);
                    }
                    else
                    dataValue+=buff;
                    buff.clear();
                    break;
                }
                buff+=ch;
                if(++ this->index == this->boundary_length)
                {
                    this->state = s_part_data_almost_end;
                }
                break;
            case s_part_data_almost_end:
                //cerr << "s_part_data_almost_end" << endl;
                //cout << dataValue << "<br>";
                if(file)
                {
                    fclose(f);
                    f = NULL;
                    newFile(paramName, fileName, type, tmpName, fileSize(tmpName));
                }
                _POST.push_back(_postItemData());
                _POST.back().paramName = getpostParamNameValue(findValueByName("Content-Disposition", headers));
                _POST.back().data = dataValue;
                headers.clear();
                if (ch == '-') 
                {
                    this->state = s_part_data_final_hyphen;
                    break;
                }
                if (ch == CR) 
                {
                    this->state = s_part_data_end;
                    break;
                }
                return;
            case s_part_data_final_hyphen:
                //cerr << "s_part_data_final_hyphen" << endl;
                if (ch == '-') 
                {
                    //последний boundary закончился
                    this->state = s_end;
                    break;
                }
                return;
            case s_part_data_end:
                //cerr << "s_part_data_end" << endl;
                //data собрано 
                //cout << dataValue << "<br>";
                if (ch == LF) 
                {
                    this->state = s_header_field_start;
                    break;
                }
                return;
            case s_end:
                //The End!
                //cerr << "s_end" << endl;
                //cout << dataValue << "<br>";
                return;
            default:
                //Все очень плохо
                //cerr << "Default" << endl;
                return;
        }
        //if(ch == '\n') cout << "<span style=\"color: red;\">\\n<br></span>";
        //if(ch == '\r') cout << "<span style=\"color: red;\">\\r</span>";
        //cout << ch;
    }
}

string HTTP::generateTmpName() 
{
    string str;
    const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
    for(int i = 0; i < 8; i++) str+=alphanum[rand() % (sizeof(alphanum)-1)];
    return str;
}

void HTTP::newFile(string paramName, string filename, string type, string tmpName, size_t size) 
{
    this->_Files.push_back(_userFile());
    this->_Files.back().fileName = filename;
    this->_Files.back().paraName = paramName;
    this->_Files.back().tmpName = tmpName;
    this->_Files.back().size = size;
    this->_Files.back().type = type;
    this->_Files.back().error = UPLOAD_ERR_OK;
}

string HTTP::getpostFileNameValue(string str) 
{
    size_t found;
    if((found = str.find("filename=\"")) == string::npos)
    {
        //cerr << "can't find filename in " << str << endl;
        return string();
    }
    if(str.c_str()[found-1] == '"') return string();
    //cerr << "find filename in " << str << endl;
    const char*tmp = str.c_str()+found+sizeof("filename")+1;
    const char*start = tmp;
    while(*tmp++) ;
    string fileName(str, start - str.c_str(), tmp-start-3);
    //cerr << fileName << endl;
    return fileName;
}

string HTTP::getpostParamNameValue(string str) 
{
    size_t found;
    if((found = str.find("name=\"")) == string::npos)
    {
        //cerr << "can't find name in " << str << endl;
        return string();
    }
    if(str.c_str()[found-1] == '"') return string();
    //cerr << "find name in " << str << endl;
    const char*tmp = str.c_str()+found+sizeof("name")+1;
    const char*start = tmp;
    while(*tmp) if(*tmp++ == ';') break;
    string name(str, start - str.c_str(), tmp-start-2);
    //cerr << name << endl;
    return name;
}

void HTTP::addPostItem(string paramName, string value) 
{
    this->_POST.push_back(_postItemData());
    this->_POST.back().paramName = paramName;
    this->_POST.back().data = value;
    //cerr <<  "add to post vector " << this->_POST.back().paramName << " = " << this->_POST.back().data << endl;
}

size_t HTTP::fileSize(string fileName) 
{
    FILE*  fMyFile = fopen(fileName.c_str(),"r");  
    fseek(fMyFile,0,SEEK_END);  
    size_t size = ftell(fMyFile); 
    return size;
}

void HTTP::print_Files() 
{
    cout << "fileCount = " << this->_Files.size() << endl;
    for(int i = 0; i < this->_Files.size(); i++)
    {

        cout << "paramName = " << this->_Files[i].paraName << "<br>";
        cout << "fileName = " << this->_Files[i].fileName << "<br>";
        cout << "tmpFileName = " << this->_Files[i].tmpName << "<br>";
        cout << "typw = " << this->_Files[i].type << "<br>";
        cout << "size = " << this->_Files[i].size << "<br>";
        cout << "error = " << this->_Files[i].error << "<br>";
        cout << "===<br>";
    }
}

filesVector HTTP::filesArray(string name) 
{
    filesVector tmp;
    for(int i = 0; i < this->_Files.size(); i++)
        if(this->_Files[i].paraName == name) tmp.push_back(this->_Files[i]);
    return tmp;
}

getVector HTTP::getArray(string paramName) 
{
    getVector tmp;
    for(int i = 0; i<this->_GET.size();i++) 
        if(this->_GET[i].paramName == paramName) 
            tmp.push_back(this->_GET[i].value);
    return tmp;
}

postVector HTTP::postArray(string paramName) 
{
    postVector tmp;
    for(int i = 0; i<this->_POST.size();i++) 
        if(this->_POST[i].paramName == paramName) 
            tmp.push_back(this->_POST[i].data);
    return tmp;
}

void HTTP::setHTTPStatus(int type) 
{
    setHeader("Status", getResponseCode(type));
}

void HTTP::setHTTPStatus(int code, string text) 
{
    setHeader("Status", this->NumberToString(code)+text);
}

string HTTP::getCookie(string paramName) 
{
    for(int i = 0; i < this->_Cookie.size(); i++) if(this->_Cookie[i].paramName == paramName) return this->_Cookie[i].value;
    return "";
}

void HTTP::loadCookie(string str) 
{
    this->_Cookie.clear();
    if(str.empty()) return;
    const char* start = str.c_str();
    const char *curr = start, *tmp = start;
    do
    {
        if(*curr == '=')
        {
            tmp = curr;
            continue;
        }
        if(*curr == ';' || *curr == '\0')
        {
            if(curr == start) return;
            string paramName(start, tmp-start);
            string value(tmp+1, curr-tmp-1);
            this->_Cookie.push_back(_cookieItemData());
            this->_Cookie.back().paramName = paramName;
            this->_Cookie.back().value = value;
            start = curr+2;
        }
    }while(*curr++);
}

void HTTP::redirect(string url) 
{
    setHTTPStatus(status_302_Moved_Temporarily);
    setHeader("Location", url);
}

void HTTP::redirectPermanent(string url) 
{
    setHTTPStatus(status_301_Moved_Permanently);
    setHeader("Location", url);
}
string HTTP::getResponseCode(int iResponseCode)
{
	switch (iResponseCode)
	{
		case 100: return "100 Continue";
		case 101: return "101 Switching Protocols";
		case 200: return "200 OK";
		case 201: return "201 Created";
		case 202: return "202 Accepted";
		case 203: return "203 Non-Authoritative Information";
		case 204: return "204 No Content";
		case 205: return "205 Reset Content";
		case 206: return "206 Partial Content";
		case 300: return "300 Multiple Choices";
		case 301: return "301 Moved Permanently";
		case 302: return "302 Found";
		case 303: return "303 See Other";
		case 304: return "304 Not Modified";
		case 305: return "305 Use Proxy";
		case 307: return "307 Temporary Redirect";
		case 400: return "400 Bad Request";
		case 401: return "401 Unauthorized";
		case 402: return "402 Payment Required";
		case 403: return "403 Forbidden";
		case 404: return "404 Not Found";
		case 405: return "405 Method Not Allowed";
		case 406: return "406 Not Acceptable";
		case 407: return "407 Proxy Authentication Required";
		case 408: return "408 Request Timeout";
		case 409: return "409 Conflict";
		case 410: return "410 Gone";
		case 411: return "411 Length Required";
		case 412: return "412 Precondition Failed";
		case 413: return "413 Request Entity Too Large";
		case 414: return "414 Request-URI Too Long";
		case 415: return "415 Unsupported Media Type";
		case 416: return "416 Requested Range Not Satisfiable";
		case 417: return "417 Expectation Failed";
		case 418: return "418 I'm a teapot";
		case 422: return "422 Unprocessable Entity (WebDAV) (RFC 4918)";
		case 423: return "423 Locked (WebDAV) (RFC 4918)";
		case 424: return "424 Failed Dependency (WebDAV) (RFC 4918)";
		case 425: return "425 Unordered Collection (RFC 3648)";
		case 426: return "426 Upgrade Required (RFC 2817)";
		case 449: return "449 Retry With";
		case 450: return "450 Blocked by Windows Parental Controls";
                case 451: return "451 Unavailable For Legal Reasons";
		case 500: return "500 Internal Server Error";
		case 501: return "501 Not Implemented";
		case 502: return "502 Bad Gateway";
		case 503: return "503 Service Unavailable";
		case 504: return "504 Gateway Timeout";
		case 505: return "505 HTTP Version Not Supported";
		case 506: return "506 Variant Also Negotiates (RFC 2295)";
		case 507: return "507 Insufficient Storage (WebDAV) (RFC 4918)";
		case 509: return "509 Bandwidth Limit Exceeded (Apache bw/limited extension)";
		case 510: return "510 Not Extended (RFC 2774)";
		case 600: return "600 Malformed URI";
		case 601: return "601 Connection Timed";
		case 602: return "602 Unknown Error";
		case 603: return "603 Could Not Parse Reply";
		case 604: return "604 Protocol Not Supported";
		default:
			return "500 Internal Server Error";
	}
}

void HTTP::printAllEnv() 
{
    this->_body_buff += "Ev list:</br>";
    for(int i = 0; i < this->_Env.size(); i++)
        this->_body_buff += " : " + this->_Env[i].paramName + " - " + this->_Env[i].value + "</br>";
}

void HTTP::attachStreams(){
//    fcgi_istream cin;
//    fcgi_ostream cout;
//    fcgi_ostream cerr;

    cin_stream.attach(request.in);
    cout_stream.attach(request.out);
    cerr_stream.attach(request.err);

}

string HTTP::findValueByName(string name, vector<_headerPostItemData> vec) {
    for (int i = 0; i < vec.size(); i++){
        if (vec[i].paramName == name){
            return vec[i].value;
        }
    }
    return "";
}

bool HTTP::isInProcess() {
    return processing;
}

void HTTP::setSession(string paramName, string paramValue) {
    setCookie(paramName, paramValue, time(NULL) + 600, "/", /*getEnv("HTTP_HOST")*/"localhost" );
}

string HTTP::getSession(string paramName) {
    return getCookie(paramName);
}

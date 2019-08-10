//
// Created by leptis on 3/14/18.
//

#ifndef INTERPRETER_FCGIHANDLER_H
#define INTERPRETER_FCGIHANDLER_H

#include "HTTP.h"
#include <string>
#include <queue>

struct ServerParams{
    std::string port;
    std::string ip;
    unsigned int thread_count;
    unsigned int sock_id;
    std::queue<HTTP*> rh_queue;
    ServerParams(std::string new_ip, std::string new_port, unsigned int new_thread_count);

};

class FCGIHandler{
public:
    FCGIHandler() = default;
    explicit FCGIHandler(ServerParams new_params) : params(new_params){};
    //void getParam();
    inline void setParams(ServerParams new_params){params = new_params;};
    void StartServer();
    HTTP *getRequest();
    void  putRequest(HTTP *request);
    void uploadResponse(HTTP *to, std::string html_response, std::string html_header_response = "");
private:
    pthread_t mother_process;
    class SocketOpenError{};

    ServerParams params;

    static void *fcgiInit(void *a);
    static void *doit(void *a);
};


#endif //INTERPRETER_FCGIHANDLER_H

//
// Created by leptis on 3/14/18.
//
#include <iostream>
#include "fcgiHandler.h"

#include <fcgio.h>

#include <fcgi_stdio.h>
#include <fstream>
#include <iconv.h>
#include "HTTP.h"



void FCGIHandler::StartServer() {
    pthread_create(&mother_process, nullptr, fcgiInit, &(this->params));
    pthread_detach(mother_process);

}


void *FCGIHandler::fcgiInit(void *a) {
    //FILE *file = fopen("/home/zombie/BLOCKSET/interpreter/log", "w");
    //fputs("fcgi uploaded\n", file);
    //fclose(file);

    ServerParams *params = (ServerParams*) a;

    int i;
    pthread_t id[(params->thread_count)];

    //инициализация библилиотеки
    FCGX_Init();
    printf("Lib is inited\n");

    //открываем новый сокет
    try {
        params->sock_id = FCGX_OpenSocket((params->ip + ":" + params->port).c_str(), 20);
        if (params->sock_id < 0) {
            throw SocketOpenError();
        }
    }
    catch (SocketOpenError) {
        //Обрабатываем
    }


    printf("Socket is opened\n");

    //создаём рабочие потоки
    for (i = 0; i < params->thread_count; i++) {
        pthread_create(&id[i], nullptr, doit, params);

    }

    //ждем завершения рабочих потоков
    for (i = 0; i < params->thread_count; i++) {
        pthread_join(id[i], nullptr);
    }
    return nullptr;
}

void *FCGIHandler::doit(void *a)
{
    ServerParams *params = (ServerParams*) a;

    HTTP request_handler = HTTP(params->sock_id); // Создали обработчик запроса и поинитили переменную запроса

    //cout << "Request is inited" << endl;

    for(;;)
    {



        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
        static pthread_mutex_t rh_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

        //попробовать получить новый запрос
        cout << "Try to accept new request" << endl;

        FILE *file;
        file = fopen("log", "a");
        fputs("Try to accept new request\n", file);
        fclose(file);

        while (request_handler.isInProcess()); // Ожиданием пока этот обработчик закончит обработку предыдущего запроса

        pthread_mutex_lock(&accept_mutex);
        request_handler.acceptConnection(); // Ожидаем запроса
        pthread_mutex_unlock(&accept_mutex);


        cout << "request is accepted" << endl;

        request_handler.loadEnvItems(); // Подгрузили окружение


        request_handler.attachStreams();
        try {
            request_handler.loadPostData();
        }
        catch(...){
        }


        pthread_mutex_lock(&rh_queue_mutex);
        params->rh_queue.push(&request_handler);    // Засовываем запрос в единую очередь, а которой они ждут обработки
        pthread_mutex_unlock(&rh_queue_mutex);

    }

    return nullptr;
}

void FCGIHandler::uploadResponse(HTTP *to, std::string html_response, std::string html_header_response) {

    FILE *file;

    //        if (!to->filesArray("f1").empty()){
//            //to->print_Files();
//            cout << "yes" << endl;
//        }else{
//            cout << "no" << endl;
//        }

    file = fopen("log", "a");
    fputs("request is accepted\n", file);
    fclose(file);

    //hile (1); // Ожидание обработки запроса

    //to->print_Files();

    //получить значение переменной
    if (html_header_response == "") {
//        std::string server_name = to->getEnv("REQUEST_URI");
//        std::string server_url = FCGX_GetParam("REQUEST_URI", request.envp);
//        std::string method = //FCGX_GetParam("REQUEST_METHOD", request.envp);
//        FCGX_PutS("Content-type: text/html; charset=utf-8 \n", to->getCout());
//
//        FCGX_PutS("\n", to->getCout());
    }else{
//        FCGX_PutS((html_header_response + "\n").c_str(), to->getCout());
    }
    //вывести тело ответа (например - html-код веб-страницы)
//    FCGX_PutS("<html>\n", to->getCout());
//    FCGX_PutS("<head>\n", to->getCout());
//    FCGX_PutS("<title>FastCGI Hello! (multi-threaded C, fcgiapp library)</title>\n", to->getCout());
//    FCGX_PutS("</head>\n", to->getCout());
//    FCGX_PutS("<body>\n", to->getCout());
//    FCGX_PutS("<h1>FastCGI ЫЫЫЫЫЫЫЫ! (multi-threaded C, fcgiapp library)</h1>\n", to->getCout());
//    FCGX_PutS(to->getEnv("REQUEST_URI").c_str(), to->getCout());
//    FCGX_PutS(to->getEnv("REQUEST_METHOD").c_str(), to->getCout());
//    FCGX_PutS("<p>Request accepted from host <i>", to->getCout());
//    FCGX_PutS((!server_name.empty()) ? server_name.c_str() : "?", to->getCout());
//    FCGX_PutS("</i></p>\n", to->getCout());
//    FCGX_PutS("</body>\n", to->getCout());
//    FCGX_PutS("</html>\n", to->getCout());

//    std::string html_local_addr = /*"test/base.html"*/ html_response;
//    std::string html_code;
    //html = function(server_url);  ----------------->>> Функция которая получает адрес html документа по урлу <<<----------------- ВАЖНО

    //FILE *html_doc = fopen(html_local_addr.c_str(), "r");

    /*int i = 0;

    std::ifstream html_doc;

    html_doc.open(html_local_addr, std::ios::in);

    while (std::getline(html_doc, html_code)){

        FCGX_PutS(html_code.c_str(), to->getCout());
    }*/

//    FCGX_PutS(html_response.c_str(), to->getCout());

    to->freeConnection();
}


HTTP *FCGIHandler::getRequest() {
    if (params.rh_queue.size() > 0) {
        HTTP *rh = params.rh_queue.front();
        params.rh_queue.pop();
        return rh;
    }else{
        return nullptr;
    }

}

void FCGIHandler::putRequest(HTTP *request) {
    params.rh_queue.push(request);
    return;
}


ServerParams::ServerParams(std::string new_ip, std::string new_port, unsigned int new_thread_count) {
    ip = new_ip;
    port = new_port;
    thread_count = new_thread_count;
}

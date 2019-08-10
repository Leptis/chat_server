#include <iostream>
#include "../fastCGI/fcgiHandler.h"


int main() {

    FCGIHandler *server_handler = new FCGIHandler(ServerParams("*", "9000", 1));
    server_handler->StartServer();
    HTTP
//    std::cout << "Hello, World!" << std::endl;
    return 0;
}
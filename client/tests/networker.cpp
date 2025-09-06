#include "rat/networking.hpp"

int main(void) {
    const std::string url = "https://upload.wikimedia.org/wikipedia/commons/3/3e/Rose_Cleveland.jpg";
    rat::networking::downloadFile(url, "hello.jpg");

    return 0;
}
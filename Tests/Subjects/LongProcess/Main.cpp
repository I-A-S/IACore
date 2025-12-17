#include <thread>
#include <iostream>

int main(int, char **)
{
    std::cout << "Started!\n";
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Ended!\n";
    std::cout.flush();
    return 100;
}
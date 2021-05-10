#include "system/manager.hpp"

int main()
{
    std::shared_ptr<Manager> manager(new Manager);

    while (manager->window->isOpen())
        manager->update();
    
    return 0;
}
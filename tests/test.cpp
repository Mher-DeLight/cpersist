#include <cpersist.h>
#include <iostream>

int main()
{
    cpersist_internal::ErrorManager::get().throwWarning("you have been warned.");
    cpersist_internal::ErrorManager::get().assert(false, "it's true.");

    return 0;
}
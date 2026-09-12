#include "UniquePtr.hpp"
#include <string>

int main(){
    UniquePtr<std::string> nn = UniquePtr<std::string>::make_unique("Вася");
}
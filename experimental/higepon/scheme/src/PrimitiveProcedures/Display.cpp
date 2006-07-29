#include "Display.h"

using namespace monash;

Display::Display()
{
}

Display::~Display()
{
}

std::string Display::toString()
{
    return "procedure:display";
}

Object* Display::eval(Environment* env)
{
    printf("don't eval me");
    return NULL;
}

Object* Display::apply(Objects* arguments)
{
    if (arguments->size() != 1)
    {
        printf("display got error");
        return NULL;
    }
    printf(arguments->at(0)->toString().c_str());
    return new Number(0); // hutei
}
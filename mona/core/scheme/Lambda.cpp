/*!
    \file   Lambda.cpp
    \brief

    Copyright (c) 2002-2007 Higepon.
    All rights reserved.
    License=MIT/X License

    \author  Higepon
    \version $Revision$
    \date   create:2007/07/14 update:$Date$
*/
#include "Lambda.h"

using namespace util;
using namespace monash;

// lambda (x)     : normal parameter
// lambda x       : extendableParameter. x is list.
// lambda (x . y) : extendableParameters. y is list.
Lambda::Lambda(Objects* body, Variables* parameters, bool isExtendableParameter, bool isMacro, uint32_t lineno)
    : body_(body), parameters_(parameters), isExtendableParameter_(isExtendableParameter), isExtendableParameters_(false), isMacro_(isMacro), lineno_(lineno)
{
    for (int i = 0; i < parameters->size(); i++)
    {
        if (parameters->get(i)->name() == ".")
        {
            isExtendableParameters_ = true;
        }
    }
}

Lambda::~Lambda()
{
}

String Lambda::toString()
{
    return "lambda : " + body_->get(0)->toString();
}

int Lambda::type() const
{
    return Object::LAMBDA;
}

Object* Lambda::eval(Environment* env)
{
    Object* procedure = new Procedure(this, env, isMacro_);
    SCM_ASSERT(procedure);
    return procedure;
}

bool Lambda::eqv() const
{
    return false;
}

bool Lambda::eq() const
{
    return false;
}

#include "Translator.h"

using namespace monash;

Translator::Translator()
{
}

Translator::~Translator()
{
}

int Translator::translatePrimitive(Node* node, Object** object)
{
    switch(node->type)
    {
    case Node::NUMBER:
        *object = new Number(node->value);
        return SUCCESS;
    case Node::STRING:
        *object = new String(node->text);
        return SUCCESS;
    case Node::QUOTE:
        *object = new Quote(node->text);
        return SUCCESS;
    case Node::SYMBOL:
        *object = new Variable(node->text);
        return SUCCESS;
    }
    return SYNTAX_ERROR;
}

int Translator::translateDefinition(Node* node, Object** object)
{
    if (node->nodes.size() != 3) return SYNTAX_ERROR;
    Node* symbol = node->nodes[1];
    if (symbol->type != Node::SYMBOL) return SYNTAX_ERROR;
    Variable* variable = new Variable(symbol->text);
    Node* argument = node->nodes[2];
    Object* argumentObject;
    if (translate(argument, &argumentObject) != SUCCESS) return SYNTAX_ERROR;
    *object = new Definition(variable, argumentObject);
    return SUCCESS;
}

int Translator::translateIf(Node* node, Object** object)
{
    if (node->nodes.size() != 4) return SYNTAX_ERROR;
    Object* predicate;
    Object* consequent;
    Object* alternative;
    int ret = translate(node->nodes[1], &predicate);
    if (ret != SUCCESS) return ret;
    ret = translate(node->nodes[2], &consequent);
    if (ret != SUCCESS) return ret;
    ret = translate(node->nodes[3], &alternative);
    if (ret != SUCCESS) return ret;
    *object = new SpecialIf(predicate, consequent, alternative);
    return SUCCESS;
}

int Translator::translateBegin(Node* node, Object** object)
{
    if (node->nodes.size() <= 1) return SYNTAX_ERROR;
    Objects* objects = new Objects;
    for (int i = 1; i < node->nodes.size(); i++)
    {
        Object * object;
        int ret = translate(node->nodes[i], &object);
        printf("%s %s:%d\n", __func__, __FILE__, __LINE__);fflush(stdout);// debug
        if (ret != SUCCESS) return ret;
        printf("%s %s:%d\n", __func__, __FILE__, __LINE__);fflush(stdout);// debug
        objects->push_back(object);
    }
    *object = new Begin(objects);
    return SUCCESS;
}

int Translator::translateLambda(Node* node, Object** object)
{
    if (node->nodes.size() <= 2) return SYNTAX_ERROR;
    if (node->nodes[1]->type != Node::NODES) return SYNTAX_ERROR;
    Variables* variables = new Variables;
    for (int i = 0; i < node->nodes[1]->nodes.size(); i++)
    {
        Node* param = node->nodes[1]->nodes[i];
        if (param->type != Node::SYMBOL) return SYNTAX_ERROR;
        variables->push_back(new Variable(param->text));
    }

    Objects* body = new Objects;
    for (int i = 2; i < node->nodes.size(); i++)
    {
        Object* o;
        int ret = translate(node->nodes[i], &o);
        if (ret != SUCCESS) return ret;
        body->push_back(o);
    }
    *object = new Lambda(body, variables);
    return SUCCESS;
}

int Translator::translateApplication(Node* node, Object** object)
{
    Object* f;
    int ret = translate(node->nodes[0], &f);
    Objects* arguments = new Objects;
    for (int i = 1; i < node->nodes.size(); i++)
    {
        Object * object;
        int ret = translate(node->nodes[i], &object);
        if (ret != SUCCESS) return ret;
        arguments->push_back(object);
    }
    *object = new Application(f, arguments);
    return SUCCESS;
}

int Translator::translate(Node* node, Object** object)
{
    if (node->type != Node::NODES)
    {
        return translatePrimitive(node, object);
    }

    if (node->nodes.size() <= 0) return SYNTAX_ERROR;

    Node* function = node->nodes[0];
    if (function->type == Node::SYMBOL)
    {
        if (function->text == "define")
        {
            return translateDefinition(node, object);
        }
        else if (function->text == "if")
        {
            return translateIf(node, object);
        }
        else if (function->text == "begin")
        {
            printf("%s %s:%d\n", __func__, __FILE__, __LINE__);fflush(stdout);// debug
            return translateBegin(node, object);
        }
        else if (function->text == "lambda")
        {
            return translateLambda(node, object);
        }
        else
        {
            return translateApplication(node, object);
        }
    }
    else
    {
        printf("%s %s:%d\n", __func__, __FILE__, __LINE__);fflush(stdout);// debug
        return translateApplication(node, object);
    }
    return SYNTAX_ERROR;
}

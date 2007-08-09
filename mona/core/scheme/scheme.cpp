/*!
    \file   scheme.cpp
    \brief

    Copyright (c) 2002-2007 Higepon.
    All rights reserved.
    License=MIT/X License

    \author  Higepon
    \version $Revision$
    \date   create:2007/07/14 update:$Date$
*/
#define GLOBAL_VALUE_DEFINED
#include "scheme.h"

using namespace util;
using namespace monash;

#include "MonaTerminal.h"
#include "primitive_procedures.h"

void scheme_init()
{
#ifdef USE_MONA_GC
    gc_init();
#endif

#ifndef MONA
    scheme_expand_stack(64);
#endif
    cont_initialize();
}

void scheme_const_init()
{
    g_defaultOutputPort = new OutputPort(stdout);
    g_defaultInputPort  = new InputPort("stdin", stdin);
    g_currentInputPort  = g_defaultInputPort;
    g_currentOutputPort = g_defaultOutputPort;
    g_transcript        = NULL;
    g_dynamic_winds     = new DynamicWinds;
    g_true              = new True;
    g_false             = new False;
    g_undef             = new Undef;
    g_no_arg            = new Objects;
    g_nil               = new Nil;
    g_eof               = new Eof;
}

void scheme_register_primitives(Environment* env)
{
    scheme_const_init();
#include "register.inc"
    env->defineVariable(new Variable("#f"),   g_false);
    env->defineVariable(new Variable("#t"),   g_true);
    env->defineVariable(new Variable("set!"), new Set());
}

void scheme_expand_stack(uint32_t mb)
{
#ifndef MONA
    struct rlimit r;
    getrlimit(RLIMIT_STACK, &r);
    r.rlim_cur = mb * 1024 * 1024;
    setrlimit(RLIMIT_STACK, &r);
    getrlimit(RLIMIT_STACK, &r);
#endif
}

Object* scheme_eval_string(const String& input, Environment* env, bool out /* = false */)
{
    QuoteFilter quoteFilter;
    String ret = quoteFilter.filter(input);
    StringReader* reader = new StringReader(ret);
    Scanner* scanner = new Scanner(reader);
    ExtRepParser parser(scanner);
    Object* evalFunc = (new Variable("eval"))->eval(env);
    Object* evaluated = NULL;
    for (Object* sexp = parser.parse(); sexp != SCM_EOF; sexp = parser.parse())
    {
        SCM_EVAL(evalFunc, env, evaluated, sexp);
        if (out) SCHEME_WRITE(stdout, "%s\n", evaluated->toString().data());
    }
    return evaluated;
}

Environment* env;

void timerFunction()
{
    scheme_eval_string("(mona-timer-iteration)", env, false);
}


int scheme_batch(const String& file)
{
    g_batch_mode = true;
    String input = load(file);
    if (input == "")
    {
        fprintf(stderr, "can not load: %s file\n", file.data());
        return -1;
    }
    Error::exitOnError();
    Error::file = file;
    MacroFilter f;
    Translator translator;
    Environment* env = new Environment(f, translator);
    SCM_ASSERT(env);
    g_top_env = env;
    scheme_register_primitives(env);
    scheme_eval_string(LOAD_SCHEME_BATCH_LIBRARY, env, false);
    scheme_eval_string(input, env);
    return 0;
}

void scheme_interactive()
{
    MacroFilter f;
    Translator translator;
    env = new Environment(f, translator);
    Interaction* interaction = new Interaction(env);
#ifdef MONA
    g_terminal = new monash::MonaTerminal(timerFunction);
#endif
    SCM_ASSERT(env);
    g_top_env = env;
    scheme_register_primitives(env);
    RETURN_ON_ERROR("stdin");
    scheme_eval_string(LOAD_SCHEME_INTERACTIVE_LIBRARY, env, false);

    interaction->showPrompt();

#ifdef MONA
    set_timer(100);
    for (;;)
    {
        // insert '(' automatically.y
        g_terminal->outputChar('(');
        interaction->onInput(g_terminal->getLine());
    }
#else

    for (;;)
    {
        char* line = NULL;;
        size_t length = 0;
        getline(&line, &length, stdin);
        interaction->onInput(line);
    }
#endif
}
SExp* objectToSExp(Object* o)
{
    SExp* sexp = NULL;
    if (o->isNumber())
    {
        sexp = new SExp(SExp::NUMBER, o->lineno());
        Number* n = (Number*)o;
        sexp->value = n->value();
    }
    else if (o->isNil())
    {
        sexp = new SExp(SExp::SEXPS, o->lineno());
    }
    else if (o->isCharcter())
    {
        sexp = new SExp(SExp::CHAR, o->lineno());
        Charcter* c = (Charcter*)o;
        sexp->text = c->stringValue();

    }
    else if (o->isVector())
    {
        sexp = new SExp(SExp::SEXPS, o->lineno());
        ::monash::Vector* v = (::monash::Vector*)o;
        SExp* vectorStart = new SExp(SExp::SYMBOL, o->lineno());
        vectorStart->text = "vector";
        sexp->sexps.add(vectorStart);
        for (uint32_t i = 0; i < v->size(); i++)
        {
            sexp->sexps.add(objectToSExp(v->get(i)));
        }
    }
    else if (o->isSString())
    {
        sexp = new SExp(SExp::STRING, o->lineno());
        SString* s = (SString*)o;
        sexp->text = s->value();
    }
    else if (o->isVariable())
    {
        sexp = new SExp(SExp::SYMBOL, o->lineno());
        Variable* v = (Variable*)o;
        sexp->text = v->name();
    }
    else if (o->isRiteralConstant())
    {
        sexp = new SExp(SExp::SYMBOL, o->lineno());
        RiteralConstant* r = (RiteralConstant*)o;
        sexp->text = r->text();
    }
    else if (o->isPair())
    {
        ::monash::Pair* p = (::monash::Pair*)o;
        sexp = pairToSExp(p);
    }
    else if (o->isTrue())
    {
        sexp = new SExp(SExp::SYMBOL, o->lineno());
        sexp->text = "#t";
    }
    else if (o->isFalse())
    {
        sexp = new SExp(SExp::SYMBOL, o->lineno());
        sexp->text = "#f";
    }
    else if (o->isSRegexp())
    {
        SRegexp* r = (SRegexp*)o;
        sexp = new SExp(SExp::REGEXP, o->lineno());
        sexp->text = r->pattern();
        sexp->value = r->isCaseFold() ? 1 : 0;
    }
    else
    {
        RAISE_ERROR(o->lineno(), "objectToSExp error %s\n", o->typeString().data());
    }
    return sexp;
}

SExp* pairToSExp(monash::Pair* p)
{
    SExp* sexp = new SExp(SExp::SEXPS);
    Object* o = p;
    for (;;)
    {
        if (o->isNil())
        {
            break;
        }
        else if (o->isPair())
        {
            ::monash::Pair* p = (::monash::Pair*)o;
            sexp->sexps.add(::objectToSExp(p->getCar()));
            if (!p->getCdr()->isPair() && !p->getCdr()->isNil())
            {
                SExp* s = new SExp(SExp::SYMBOL);
                s->text = ".";
                sexp->sexps.add(s);
                sexp->sexps.add(::objectToSExp(p->getCdr()));
                break;
            }
            o = p->getCdr();
        }
        else
        {
            sexp->sexps.add(::objectToSExp(o));
            break;
        }
    }
    return sexp;
}

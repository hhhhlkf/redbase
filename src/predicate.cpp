#include "predicate.h"
#include <cstdio>
#include <cstring>
#include <cassert>
bool Predicate::eval(const char *buf, CompOp c) const
{
    return this->eval(buf, NULL, c);
}
bool Predicate::eval(const char *lhsbuf, const char *rhsbuf, CompOp c) const
{
    const void *value_ = rhsbuf;
    if (rhsbuf == NULL)
        value_ = value;
    if (c == NO_OP || value_ == NULL)
    {
        return true;
    }
    const char *attr = lhsbuf + attrOffset;
    if (c == LT_OP)
    {
        if (attrType == INT)
        {
            return *((int *)attr) < *((int *)value_);
        }
        if (attrType == FLOAT)
        {
            return *((float *)attr) < *((float *)value_);
        }
        if (attrType == STRING)
        {
            return strncmp(attr, (char *)value_, attrLength) < 0;
        }
    }
    if (c == GT_OP)
    {
        if (attrType == INT)
        {
            return *((int *)attr) > *((int *)value_);
        }
        if (attrType == FLOAT)
        {
            return *((float *)attr) > *((float *)value_);
        }
        if (attrType == STRING)
        {
            return strncmp(attr, (char *)value_, attrLength) > 0;
        }
    }
    if (c == LE_OP)
    {
        return this->eval(lhsbuf, rhsbuf, LT_OP) || this->eval(lhsbuf, rhsbuf, EQ_OP);
    }
    if (c == GE_OP)
    {
        return this->eval(lhsbuf, rhsbuf, GT_OP) || this->eval(lhsbuf, rhsbuf, EQ_OP);
    }
    if (c == NE_OP)
    {
        return !this->eval(lhsbuf, rhsbuf, EQ_OP);
    }
    assert("Bad value for c - should never get here.");
    return true;
}
#include <Magick++.h>
#include <node.h>
#include <node_buffer.h>
#include <v8.h>
#include "nan.h"

using namespace v8;
using namespace node;

// Base class of command
class ImCommandWrapper {
public:
    ImCommandWrapper() {};
    virtual ~ImCommandWrapper() {};
    virtual bool loadConfig(Local<Object> v) {
        return true;
    };
    virtual bool execute(Magick::Image *image) {
        return true;
    }
};

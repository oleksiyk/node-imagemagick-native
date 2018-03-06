
void DoImPipeline(uv_work_t* req) {
    convert_im_ctx* context = static_cast<convert_im_ctx*>(req->data);

    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);
    LocalResourceLimiter limiter;

    int debug = context->debug;

    if (debug) printf( "debug: on\n" );
    if (debug) printf( "ignoreWarnings: %d\n", context->ignoreWarnings );

    if (context->maxMemory > 0) {
        limiter.LimitMemory(context->maxMemory);
        limiter.LimitDisk(context->maxMemory); // avoid using unlimited disk as cache
        if (debug) printf( "maxMemory set to: %d\n", context->maxMemory );
    }

    Magick::Blob srcBlob( context->srcData, context->length );

    Magick::Image image;

    if ( !ReadImageMagick(&image, srcBlob, context->srcFormat, context) )
        return;

    // Run commands
    for (auto o : context->cmds) {
        o->execute(&image);
        delete o;
    }

    if( ! context->format.empty() ){
        if (debug) printf( "format: %s\n", context->format.c_str() );
        image.magick( context->format.c_str() );
    }

    Magick::Blob dstBlob;
    try {
        image.write( &dstBlob );
    }
    catch (std::exception& err) {
        std::string message = "image.write failed with error: ";
        message            += err.what();
        context->error = message;
        return;
    }
    catch (...) {
        context->error = std::string("unhandled error");
        return;
    }
    context->dstBlob = dstBlob;
}

NAN_METHOD(Pipeline) {
    Nan::HandleScope();

    if ( info.Length() < 1 ) {
        return Nan::ThrowError("pipeline() requires 1 (option) argument!");
    }

    if ( ! info[ 0 ]->IsObject() ) {
        return Nan::ThrowError("pipeline()'s 1st argument should be an object");
    }

    if( ! info[ 1 ]->IsFunction() ) {
        return Nan::ThrowError("pipeline()'s 2nd argument should be a function");
    }

    Local<Object> obj = Local<Object>::Cast( info[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( Nan::New<String>("srcData").ToLocalChecked() ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return Nan::ThrowError("pipeline()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }
    convert_im_ctx* context = new convert_im_ctx();
    context->srcData = Buffer::Data(srcData);
    context->length = Buffer::Length(srcData);
    context->debug = obj->Get( Nan::New<String>("debug").ToLocalChecked() )->Uint32Value();
    context->ignoreWarnings = obj->Get( Nan::New<String>("ignoreWarnings").ToLocalChecked() )->Uint32Value();
    context->maxMemory = obj->Get( Nan::New<String>("maxMemory").ToLocalChecked() )->Uint32Value();

    Local<Value> formatValue = obj->Get( Nan::New<String>("format").ToLocalChecked() );
    context->format = !formatValue->IsUndefined() ?
        *String::Utf8Value(formatValue) : "";

    Local<Value> srcFormatValue = obj->Get( Nan::New<String>("srcFormat").ToLocalChecked() );
    context->srcFormat = !srcFormatValue->IsUndefined() ?
        *String::Utf8Value(srcFormatValue) : "";

    context->callback = new Nan::Callback(Local<Function>::Cast(info[1]));


    Local<Array> cmds = Local<Array>::Cast( obj->Get( Nan::New<String>("cmds").ToLocalChecked() ) );

    for (unsigned int c = 0; c < cmds->Length(); c++) {
        Local<Object> cmd = Local<Object>::Cast( cmds->Get( c ) );
        int id = cmd->Get( Nan::New<String>("id").ToLocalChecked() )->Uint32Value();
        ImCommandWrapper *w = getCommandType(id);
        Local<Object> options = Local<Object>::Cast( cmd->Get( Nan::New<String>("options").ToLocalChecked() ) );
        w->loadConfig(options);
        context->cmds.push_back(w);
    }

    uv_work_t* req = new uv_work_t();
    req->data = context;

    uv_queue_work(uv_default_loop(), req, DoImPipeline, (uv_after_work_cb)GeneratedBlobAfter);
}


// .flip() command
class ImFlipCommand : public ImCommandWrapper {
public:
    bool execute(Magick::Image *image) {
        image->flip();
        return true;
    };
};

// .rotate(90) command
class ImRotateCommand : public ImCommandWrapper {
public:
    bool loadConfig(Local<Object> v) {
        this->degrees = v->IsUndefined() ? 0 : v->Int32Value();
        return true;
    };

    bool execute(Magick::Image *image) {
        image->rotate(this->degrees);
        return true;
    };
private:
    int degrees = 0;
};

// .blur(1.5) command
class ImBlurCommand : public ImCommandWrapper {
public:
    bool loadConfig(Local<Object> v) {
        this->blur = v->IsUndefined() ? 0.0 : v->NumberValue();
        return true;
    };
    bool execute(Magick::Image *image) {
        image->blur(0, this->blur);
        return true;
    };
private:
    double blur;
};

// .quality(9) command
class ImQualityCommand : public ImCommandWrapper {
public:
    bool loadConfig(Local<Object> v) {
        if (v->IsUndefined()) {
            return true;
        }
        this->quality = v->Uint32Value();
        return true;
    };

    bool execute(Magick::Image *image) {
        image->quality(this->quality);
        return true;
    };
private:
    unsigned int quality;
};

// .autoOrient() command
class ImAutoOrientCommand : public ImCommandWrapper {
public:
    bool execute(Magick::Image *image) {
        autoOrient(image);
        return true;
    };
};

// .density(75) command
class ImDensityCommand : public ImCommandWrapper {
public:
    bool loadConfig(Local<Object> v) {
        if (v->IsUndefined()) {
            return true;
        }
        this->density = v->Int32Value();
        return true;
    };

    bool execute(Magick::Image *image) {
        #if MagickLibVersion < 0x700
            image->density(Magick::Geometry(this->density, this->density));
        #else
            image->density(Magick::Point(this->density, this->density));
        #endif
        return true;
    };
private:
    int density;
};

// .colorSpace("CMYK") command
class ImColorspaceCommand : public ImCommandWrapper {
public:
    bool loadConfig(Local<Object> v) {
        if (v->IsUndefined()) {
            this->colorspace = Magick::UndefinedColorspace;
            return true;
        }

        ssize_t colorspace = -1;
        colorspace = MagickCore::ParseCommandOption(
            MagickCore::MagickColorspaceOptions,
            MagickCore::MagickFalse,
            *String::Utf8Value(v)
        );
        this->colorspace = colorspace != (-1) ? (Magick::ColorspaceType) colorspace : Magick::UndefinedColorspace;

        return true;
    };

    bool execute(Magick::Image *image) {
        if( this->colorspace != Magick::UndefinedColorspace ) {
            printf(
                "colorspace: %s\n",
                MagickCore::CommandOptionToMnemonic(
                    MagickCore::MagickColorspaceOptions,
                    static_cast<ssize_t>(this->colorspace)
                )
            );
            image->colorSpace( this->colorspace );
        }
        return true;
    }
private:
    Magick::ColorspaceType colorspace;
};

// .composite({ srcData: srcData, gravity: 'NorthGravity', op: 'OverCompositeOp' })
class ImCompositeCommand : public ImCommandWrapper {
public:
    bool loadConfig(Local<Object> v) {
        if (v->IsUndefined()) {
            return true;
        }

        this->xoffset = v->Get( Nan::New<String>("xoffset").ToLocalChecked() )->Uint32Value();
        this->yoffset = v->Get( Nan::New<String>("yoffset").ToLocalChecked() )->Uint32Value();

        Local<Value> gravityValue = v->Get( Nan::New<String>("gravity").ToLocalChecked() );
        this->gravity = !gravityValue->IsUndefined() ? *String::Utf8Value(gravityValue) : "Center";

        Local<Object> srcData = Local<Object>::Cast( v->Get( Nan::New<String>("srcData").ToLocalChecked() ) );
        if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
            // return Nan::ThrowError("1st argument should have \"srcData\" key with a Buffer instance");
            return false;
        }

        this->srcData = Buffer::Data(srcData);
        this->length = Buffer::Length(srcData);

        return true;
    };
    bool execute(Magick::Image *image) {
        Magick::GravityType gravityType;

        const char* gravity = this->gravity.c_str();

        if(strcmp("Center", gravity)==0)         gravityType=Magick::CenterGravity;
        else if(strcmp("East", gravity)==0)      gravityType=Magick::EastGravity;
        else if(strcmp("Forget", gravity)==0)    gravityType=Magick::ForgetGravity;
        else if(strcmp("NorthEast", gravity)==0) gravityType=Magick::NorthEastGravity;
        else if(strcmp("North", gravity)==0)     gravityType=Magick::NorthGravity;
        else if(strcmp("NorthWest", gravity)==0) gravityType=Magick::NorthWestGravity;
        else if(strcmp("SouthEast", gravity)==0) gravityType=Magick::SouthEastGravity;
        else if(strcmp("South", gravity)==0)     gravityType=Magick::SouthGravity;
        else if(strcmp("SouthWest", gravity)==0) gravityType=Magick::SouthWestGravity;
        else if(strcmp("West", gravity)==0)      gravityType=Magick::WestGravity;
        else {
            gravityType = Magick::ForgetGravity;
            printf( "invalid gravity: '%s' fell through to ForgetGravity\n", gravity);
        }

        printf( "gravity: %s (%d)\n", gravity,(int) gravityType);

        Magick::Image compositeImage;

        Magick::Blob compositeBlob( this->srcData, this->length );

        im_ctx_base k;

        if ( !ReadImageMagick(&compositeImage, compositeBlob, "", &k) )
            return false;

        image->composite(compositeImage, gravityType, Magick::OverCompositeOp);
        return true;
    }
private:
    unsigned int xoffset;
    unsigned int yoffset;
    std::string gravity;
    char* srcData;
    size_t length;
};

// .resize({ width: 100, height: 100 }) command
#include "resize-cmd.h"

// Factory function for commands
ImCommandWrapper *getCommandType(int t) {
    ImCommandWrapper *result = NULL;
    switch(t) {
        // This MUST be the same in pipeline.js
        case 0: result = new ImFlipCommand(); break;
        case 1: result = new ImRotateCommand(); break;
        case 2: result = new ImResizeCommand(); break;
        case 3: result = new ImBlurCommand(); break;
        case 4: result = new ImQualityCommand(); break;
        case 5: result = new ImAutoOrientCommand(); break;
        case 6: result = new ImDensityCommand(); break;
        case 7: result = new ImColorspaceCommand(); break;
        case 8: result = new ImCompositeCommand(); break;
    }
    return result;
}

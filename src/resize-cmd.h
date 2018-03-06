
class ImResizeCommand : public ImCommandWrapper {
public:
    bool loadConfig(Local<Object> v) {
        if (v->IsUndefined()) {
            return true;
        }

        this->width = v->Get( Nan::New<String>("width").ToLocalChecked() )->Uint32Value();
        this->height = v->Get( Nan::New<String>("height").ToLocalChecked() )->Uint32Value();
        this->xoffset = v->Get( Nan::New<String>("xoffset").ToLocalChecked() )->Uint32Value();
        this->yoffset = v->Get( Nan::New<String>("yoffset").ToLocalChecked() )->Uint32Value();

        Local<Value> resizeStyleValue = v->Get( Nan::New<String>("resizeStyle").ToLocalChecked() );
        this->resizeStyle = !resizeStyleValue->IsUndefined() ? *String::Utf8Value(resizeStyleValue) : "aspectfill";

        Local<Value> gravityValue = v->Get( Nan::New<String>("gravity").ToLocalChecked() );
        this->gravity = !gravityValue->IsUndefined() ? *String::Utf8Value(gravityValue) : "Center";

        return true;
    };

    bool execute(Magick::Image *image) {

        unsigned int width = this->width;
        printf( "width: %d\n", width );

        unsigned int height = this->height;
        printf( "height: %d\n", height );
        const char* resizeStyle = this->resizeStyle.c_str();
        printf( "resizeStyle: %s\n", resizeStyle );

        const char* gravity = this->gravity.c_str();
        if ( strcmp("Center", gravity)!=0
          && strcmp("East", gravity)!=0
          && strcmp("West", gravity)!=0
          && strcmp("North", gravity)!=0
          && strcmp("South", gravity)!=0
          && strcmp("NorthEast", gravity)!=0
          && strcmp("NorthWest", gravity)!=0
          && strcmp("SouthEast", gravity)!=0
          && strcmp("SouthWest", gravity)!=0
          && strcmp("None", gravity)!=0
        ) {
            printf( "gravity not supported\n" );
            return false;
        }
        printf( "gravity: %s\n", gravity );
        if ( width || height ) {
            if ( ! width  ) { width  = image->columns(); }
            if ( ! height ) { height = image->rows();    }

            // do resize
            if ( strcmp( resizeStyle, "aspectfill" ) == 0 ) {
                // ^ : Fill Area Flag ('^' flag)
                // is not implemented in Magick++
                // and gravity: center, extent doesnt look like working as exptected
                // so we do it ourselves

                // keep aspect ratio, get the exact provided size, crop top/bottom or left/right if necessary
                double aspectratioExpected = (double)height / (double)width;
                double aspectratioOriginal = (double)image->rows() / (double)image->columns();
                unsigned int xoffset = 0;
                unsigned int yoffset = 0;
                unsigned int resizewidth;
                unsigned int resizeheight;

                if ( aspectratioExpected > aspectratioOriginal ) {
                    // expected is taller
                    resizewidth  = (unsigned int)( (double)height / (double)image->rows() * (double)image->columns() + 1. );
                    resizeheight = height;
                    if ( strstr(gravity, "West") != NULL ) {
                        xoffset = 0;
                    }
                    else if ( strstr(gravity, "East") != NULL ) {
                        xoffset = (unsigned int)( resizewidth - width );
                    }
                    else {
                        xoffset = (unsigned int)( (resizewidth - width) / 2. );
                    }
                    yoffset = 0;
                }
                else {
                    // expected is wider
                    resizewidth  = width;
                    resizeheight = (unsigned int)( (double)width / (double)image->columns() * (double)image->rows() + 1. );
                    xoffset = 0;
                    if ( strstr(gravity, "North") != NULL ) {
                        yoffset = 0;
                    }
                    else if ( strstr(gravity, "South") != NULL ) {
                        yoffset = (unsigned int)( resizeheight - height );
                    }
                    else {
                        yoffset = (unsigned int)( (resizeheight - height) / 2. );
                    }
                }

                printf( "resize to: %d, %d\n", resizewidth, resizeheight );
                #if MagickLibVersion < 0x700
                Magick::Geometry resizeGeometry( resizewidth, resizeheight, 0, 0, 0, 0 );
                #else
                Magick::Geometry resizeGeometry( resizewidth, resizeheight, 0, 0 );
                #endif
                try {
                    image->zoom( resizeGeometry );
                }
                catch (std::exception& err) {
                    std::string message = "image.resize failed with error: ";
                    message            += err.what();
                    printf("%s\n", message.c_str());
                    return false;
                }
                catch (...) {
                    printf("unhandled error\n");
                    return false;
                }

                if ( strcmp ( gravity, "None" ) != 0 ) {
                    // limit canvas size to cropGeometry
                    printf( "crop to: %d, %d, %d, %d\n", width, height, xoffset, yoffset );
                    #if MagickLibVersion < 0x700
                    Magick::Geometry cropGeometry( width, height, xoffset, yoffset, 0, 0 );
                    #else
                    Magick::Geometry cropGeometry( width, height, xoffset, yoffset );
                    #endif
                    Magick::Color transparent( "transparent" );
                    // if ( strcmp( context->format.c_str(), "PNG" ) == 0 ) {
                    //     // make background transparent for PNG
                    //     // JPEG background becomes black if set transparent here
                    //     #if MagickLibVersion < 0x700
                    //     transparent.alpha( 1. );
                    //     #endif
                    // }

                    #if MagickLibVersion > 0x654
                        image->extent( cropGeometry, transparent );
                    #else
                        image->extent( cropGeometry );
                    #endif
                }

            }
            else if ( strcmp ( resizeStyle, "aspectfit" ) == 0 ) {
                // keep aspect ratio, get the maximum image which fits inside specified size
                char geometryString[ 32 ];
                sprintf( geometryString, "%dx%d", width, height );
                printf( "resize to: %s\n", geometryString );

                try {
                    image->zoom( geometryString );
                }
                catch (std::exception& err) {
                    std::string message = "image.resize failed with error: ";
                    message            += err.what();
                    printf("%s\n", message.c_str());
                    return false;
                }
                catch (...) {
                    printf("unhandled error\n");
                    return false;
                }
            }
            else if ( strcmp ( resizeStyle, "fill" ) == 0 ) {
                // change aspect ratio and fill specified size
                char geometryString[ 32 ];
                sprintf( geometryString, "%dx%d!", width, height );
                printf( "resize to: %s\n", geometryString );

                try {
                    image->zoom( geometryString );
                }
                catch (std::exception& err) {
                    std::string message = "image.resize failed with error: ";
                    message            += err.what();
                    printf("%s\n", message.c_str());
                    return false;
                }
                catch (...) {
                    printf("unhandled error\n");
                    return false;
                }
            }
             else if ( strcmp ( resizeStyle, "crop" ) == 0 ) {
                 unsigned int xoffset = this->xoffset;
                 unsigned int yoffset = this->yoffset;

                 if ( ! xoffset ) { xoffset = 0; }
                 if ( ! yoffset ) { yoffset = 0; }

                 // limit canvas size to cropGeometry
                 printf( "crop to: %d, %d, %d, %d\n", width, height, xoffset, yoffset );
                 #if MagickLibVersion < 0x700
                Magick::Geometry cropGeometry( width, height, xoffset, yoffset, 0, 0 );
                #else
                Magick::Geometry cropGeometry( width, height, xoffset, yoffset );
                #endif

                 Magick::Color transparent( "transparent" );
                 // if ( strcmp( context->format.c_str(), "PNG" ) == 0 ) {
                 //     // make background transparent for PNG
                 //     // JPEG background becomes black if set transparent here
                 //    #if MagickLibVersion < 0x700
                 //     transparent.alpha( 1. );
                 //    #endif
                 // }

                 #if MagickLibVersion > 0x654
                     image->extent( cropGeometry, transparent );
                 #else
                     image->extent( cropGeometry );
                 #endif

             }
            else {
                printf("resizeStyle not supported\n");
                return false;
            }
            printf( "resized to: %d, %d\n", (int)image->columns(), (int)image->rows() );
        }

        // image->rotate(this->degrees);
        return true;
    };
private:
    unsigned int width;
    unsigned int height;
    unsigned int xoffset;
    unsigned int yoffset;
    std::string resizeStyle;
    std::string gravity;
};

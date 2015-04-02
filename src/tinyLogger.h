// Copyright (c) 2011 Prime Focus Film.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the
// distribution. Neither the name of Prime Focus Film nor the
// names of its contributors may be used to endorse or promote
// products derived from this software without specific prior written
// permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef LOG_H
#define LOG_H

#include <iostream>


//----- Tiny log macros :
extern const char *tl_blue;
extern const char *tl_red;
extern const char *tl_yellow;
extern const char *tl_normal;

#ifdef FIELD3D_MAYA_DEBUG
   #define DEBUG(message) \
      std::cout << tl_blue << "[DEBUG] " << tl_blue << __FILE__ << "::" << __FUNCTION__ << "():" << __LINE__ << tl_normal << " : " << message << std::endl;

   #define TRACE() \
      std::cout << tl_yellow << "[TRACE] " << __FILE__ << "::" << __FUNCTION__ << "():" << __LINE__ << tl_normal << std::endl;

#else
   #define DEBUG(message)
   #define TRACE()
#endif

#ifdef FIELD3D_MAYA_LOG
   #define LOG(message) \
         std::cout<< tl_yellow << "[LOG  ] " << tl_blue << __FILE__ << "::" << __FUNCTION__ << "():" << __LINE__ << tl_normal << " : " << message << std::endl;

   #define WARNING(message) \
         std::cout<< tl_yellow << "[WARN ] " << tl_blue << __FILE__ << "::" << __FUNCTION__ << "():" << __LINE__ << tl_normal << " : " << message << std::endl;

   #define ERROR(message) \
         std::cerr<< tl_red    << "[ERROR] " << __FILE__ << "::" << __FUNCTION__ << "():" << __LINE__<< " : " << message << std::endl;
#else
   #define LOG(message)
   #define WARNING(message)
   #define ERROR(message)
#endif


#endif

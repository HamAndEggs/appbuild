#ifndef __LOGGING_H__
#define __LOGGING_H__

namespace appbuild{
//////////////////////////////////////////////////////////////////////////

enum LogType
{
    LOG_ERROR,      // Error is always on, hence zero. We use LOG_ERROR for when in shebang mode.
    LOG_INFO,       // Some information about the progress of the build.
    LOG_VERBOSE     // Full progress information to help issues with either the tool or the project file.
};


//////////////////////////////////////////////////////////////////////////
};//namespace appbuild{

#endif //__LOGGING_H__
#include <jni.h>
#include <string>
#include "hook.cpp"
#include <android/log.h>

#define  TAG "FUCK"
#define  LOGE __android_log_print (ANDROID_LOG_DEBUG,)
int __android_log_buf_write(int bufID, int prio, const char *tag, const char *msg);

static jint my_log(JNIEnv* env, jobject clazz,jint bufID, jint priority, jstring tagObj, jstring msgObj);
int __my__android_log_buf_write(int bufID, int prio, const char *tag, const char *msg){
    __android_log_print (ANDROID_LOG_DEBUG,TAG," call hook __my__android_log_buf_write");
    return __android_log_buf_write(bufID,prio,tag, msg);
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_itsec_fucklogger_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    __android_log_print (ANDROID_LOG_DEBUG,TAG,"ready call hook");
      char* soname ="/system/lib/liblog.so";
     char* methodname ="__android_log_buf_write";
    hook_call(soname, methodname, (unsigned int) &__my__android_log_buf_write);
    return env->NewStringUTF(hello.c_str());
}

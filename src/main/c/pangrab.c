#include <jni.h>
#include <jvmti.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* https://docs.oracle.com/javase/7/docs/platform/jvmti/jvmti.html#IterateThroughHeap */
/* https://docs.oracle.com/javase/7/docs/platform/jvmti/jvmti.html#onload */

bool checkLuhn(const jchar* value, jint value_length) {
    int sum = 0;
    if (value_length != 16) {
        return false;
    }
    /* luhn iterates the other way but we know the length is 16 */
    for (int i = 0; i < 16 ; ++i) {
        jchar digit = value[i];
        int value;
        if (digit < '0' | digit > '9') {
            return false;
        }
        value = digit - '0';
        /* we know length is 16 */
        /* TODO optimize */
        if (i % 2 == 0) {
            value = value * 2;
        }
        if (value > 9) {
            value = value - 9;
        }
        sum = sum + value;
    }
    return sum % 10 == 0;
}

jint JNICALL StringPrimitiveValueCallback(jlong class_tag,  jlong size, 
                                          jlong* tag_ptr,
                                          const jchar* value,
                                          jint value_length,
                                          void* user_data) {
    if (checkLuhn(value, value_length)) {
        printf("found pan\n");
    }
    return JVMTI_VISIT_OBJECTS;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    fprintf(stderr, "Agent_OnLoad not supported, only Agent_OnAttach\n");
    return -1;
}

JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* vm, char *options, void *reserved) {
    jvmtiEnv* jvmti;
    JNIEnv* env;
    jvmtiHeapCallbacks callbacks;
    jclass stringClass;
    
    if ((*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_0) != JNI_OK) {
        fprintf(stderr, "GetEnv failed\n");
        return -1;
    }
    
    /* https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html#wp23721 */
    /* typedef const struct JNINativeInterface *JNIEnv; */
    /* https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html#jniNativeInterface */
    /* typedef struct JNINativeInterface_ jniNativeInterface; */
    if ((*jvmti)->GetJNIFunctionTable(jvmti, (jniNativeInterface**)&env) != JVMTI_ERROR_NONE) {
        fprintf(stderr,"GetJNIFunctionTable failed.\n");
        return -1;
    }
    
    stringClass = (*env)->FindClass(env, "java/lang/String");
    if (stringClass == NULL) {
        fprintf(stderr, "Can't find String class\n");
        return -1;
    }
    if ((*env)->ExceptionCheck(env) == JNI_TRUE ) {
        fprintf(stderr, "Exception while looking up String class\n");
        return -1;
    }
    
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.string_primitive_value_callback = StringPrimitiveValueCallback;
    
    if((*jvmti)->IterateThroughHeap(jvmti, 0, stringClass, &callbacks, (void*) NULL) != JVMTI_ERROR_NONE) {
        fprintf(stderr,"IterateThroughHeap failed.\n");
        return -1;
    }
    
    return 0;
}


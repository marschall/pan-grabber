#include <jni.h>
#include <jvmti.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* https://docs.oracle.com/javase/7/docs/platform/jvmti/jvmti.html#IterateThroughHeap */
/* https://docs.oracle.com/javase/7/docs/platform/jvmti/jvmti.html#onload */

bool check_luhn(const jchar* value, jint value_length) {
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
    char pan_buffer[17];
                                          
    if (check_luhn(value, value_length)) {
        for (int i = 0; i < value_length; ++i) {
            /*
             * We're doing a 16bit -> 8bit conversion here but that
             * should be fine since all characters are digits (0-9)
             * and are therefore in ASCII
             */
            pan_buffer[i] = (char) value[i];
        }
        pan_buffer[16] = 0;
        printf("%s\n", pan_buffer);
    }
    return JVMTI_VISIT_OBJECTS;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    fprintf(stderr, "Agent_OnLoad not supported, only Agent_OnAttach\n");
    return -1;
}

void relinquish_capabilities(jvmtiEnv* jvmti, jvmtiCapabilities* capabilities) {
    jvmtiError error = (*jvmti)->RelinquishCapabilities(jvmti, capabilities);
    if (error != JVMTI_ERROR_NONE) {
        fprintf(stderr, "RelinquishCapabilities failed with error(%d)\n", error);
    }
}

JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* vm, char *options, void *reserved) {
    jvmtiEnv* jvmti;
    jvmtiCapabilities capabilities;
    /* JNIEnv* env; */
    jvmtiHeapCallbacks callbacks;
    /* jclass stringClass; */
    jvmtiError tiError;
    jint niError;
    
    niError = (*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_0);
    if (niError != JNI_OK) {
        fprintf(stderr, "GetEnv failed with error(%d)\n", niError);
        return -1;
    }
    
    memset(&capabilities, 0, sizeof(capabilities));
    capabilities.can_tag_objects = 1;
    tiError = (*jvmti)->AddCapabilities(jvmti, &capabilities);
    if (tiError != JVMTI_ERROR_NONE) {
        fprintf(stderr, "AddCapabilities failed with error(%d)\n", tiError);
        return -1;
    }
    
    /* https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html#wp23721 */
    /* typedef const struct JNINativeInterface *JNIEnv; */
    /* https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html#jniNativeInterface */
    /* typedef struct JNINativeInterface_ jniNativeInterface; */
    
    /*
    printf("GetJNIFunctionTable\n");
    if ((*jvmti)->GetJNIFunctionTable(jvmti, (jniNativeInterface**)&env) != JVMTI_ERROR_NONE) {
        fprintf(stderr,"GetJNIFunctionTable failed.\n");
        return -1;
    }
    
    printf("FindClass\n");
    stringClass = (*env)->FindClass(env, "java/lang/String");
    if (stringClass == NULL) {
        fprintf(stderr, "Can't find String class\n");
        return -1;
    }
    printf("ExceptionCheck\n");
    if ((*env)->ExceptionCheck(env) == JNI_TRUE ) {
        fprintf(stderr, "Exception while looking up String class\n");
        return -1;
    }
    */
    
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.string_primitive_value_callback = StringPrimitiveValueCallback;
    
    /* if((*jvmti)->IterateThroughHeap(jvmti, 0, stringClass, &callbacks, (void*) NULL) != JVMTI_ERROR_NONE) { */
    tiError = (*jvmti)->IterateThroughHeap(jvmti, 0, NULL, &callbacks, (void*) NULL);
    relinquish_capabilities(jvmti, &capabilities);
    if(tiError != JVMTI_ERROR_NONE) {
        fprintf(stderr,"IterateThroughHeap failed with error(%d)\n", tiError);
        return -1;
    }
    
    return 0;
}


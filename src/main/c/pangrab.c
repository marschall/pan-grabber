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
    char pan_buffer[19]; // 16 + CR + LF + 0x0
    FILE *fp;
                                          
    if (check_luhn(value, value_length)) {
        for (int i = 0; i < value_length; ++i) {
            /*
             * We're doing a 16bit -> 8bit conversion here but that
             * should be fine since all characters are digits (0-9)
             * and are therefore in ASCII
             */
            pan_buffer[i] = (char) value[i];
        }
        pan_buffer[16] = '\r';
        pan_buffer[17] = '\n';
        pan_buffer[18] = 0;
        fp = user_data;
        
        if (fp != NULL) {
            if (fputs(pan_buffer, fp) == EOF) {
                fprintf(stderr, "Failed to write PAN\n");
            }
        } else {
            pan_buffer[16] = 0;
            puts(pan_buffer);
        }
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

jint grap_pans(JavaVM* vm, FILE *fp) {
    jvmtiEnv* jvmti;
    jvmtiCapabilities capabilities;
    JNIEnv* env;
    jvmtiHeapCallbacks callbacks;
    jclass stringClass;
    jvmtiError tiError;
    jint niError;
    
    niError = (*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_0);
    if (niError != JNI_OK) {
        fprintf(stderr, "GetEnv (JVMTI) failed with error(%d)\n", niError);
        return -1;
    }
    
    memset(&capabilities, 0, sizeof(capabilities));
    capabilities.can_tag_objects = 1;
    tiError = (*jvmti)->AddCapabilities(jvmti, &capabilities);
    if (tiError != JVMTI_ERROR_NONE) {
        fprintf(stderr, "AddCapabilities failed with error(%d)\n", tiError);
        return -1;
    }
    
    niError = (*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6);
    if (niError != JNI_OK) {
        fprintf(stderr, "GetEnv (JNI) failed with error(%d)\n", niError);
        return -1;
    }
    
    stringClass = (*env)->FindClass(env, "java/lang/String");
    if (stringClass == NULL) {
        fprintf(stderr, "Can't find String class\n");
        return -1;
    }
    
    if ((*env)->ExceptionCheck(env) == JNI_TRUE ) {
        fprintf(stderr, "Exception while looking up java.lang.String class\n");
        return -1;
    }
    
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.string_primitive_value_callback = StringPrimitiveValueCallback;
    
    tiError = (*jvmti)->IterateThroughHeap(jvmti, 0, stringClass, &callbacks, (void*) fp);
    relinquish_capabilities(jvmti, &capabilities);
    if(tiError != JVMTI_ERROR_NONE) {
        fprintf(stderr,"IterateThroughHeap failed with error(%d)\n", tiError);
        return -1;
    }
    
    return 0;
}

JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* vm, char *options, void *reserved) {
    FILE *fp;
    jint result;
    char *filename;
    
    printf("fopen%s\n", options);
    filename = options;
    if (filename != NULL) {
        fp = fopen(filename, "a");
        if (fp == NULL) {
            fprintf(stderr,"Failed to open file %s\n", filename);
            return -1;
        }
    } else {
        fp = NULL;
    }
    
    printf("grap_pans\n");
    result = grap_pans(vm, fp);
    if (filename != NULL) {
        if (fclose(fp) != 0) {
            fprintf(stderr,"Failed to close file %s\n", filename);
        }
    }
    
    return result;
}


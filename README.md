PAN Grabber
===========

A JVM Agent that extracts PANs from running Java applications.

This is intended for research and education purposes.

The following very simple algorithm is used to detect PANs:

 * `java.lang.String` of length 16
 * passes the [Luhn check](http://en.wikipedia.org/wiki/Luhn_algorithm)

among other this leaves out the following PANs:

 * longer than 16 digits
 * stored in anything other than `java.lang.String` eg.
   * `long`
   * `BigInteger`
   * `byte[]` or `char[]`
   * `CharBuffer` or `ByteBuffer` especially direct allocated ones

It does not check the PANs against BIN ranges. It will also report PANs that start with digits other and 4 or 5.

JVMs supported:

 * HotSpot
 * JRockit (theoretically, untested)

JVMs not supported

 * J9 (lacks runtime attach API)
 * JVM embedded in other applications (libjvm eg. Eclipse)
 
Usage
-----

 * compile the project with `mvn clean package`
 * optionally start a sample application with `sh sample-application.sh`
 * run `sh pan-grabber.sh`

Cost
----

The cost of [IterateThroughHeap](https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html#IterateThroughHeap) is quite high:

> During the execution of this function the state of the heap does not change: no objects are allocated, no objects are garbage collected, and the state of objects (including held values) does not change. As a result, threads executing Java programming language code, threads attempting to resume the execution of Java programming language code, and threads attempting to execute JNI functions are typically stalled.

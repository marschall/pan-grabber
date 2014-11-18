#!/bin/bash

PAN_GRABBER_JAR_FILE=`find target/ -name "pan-grabber*.jar" | grep -v javadoc | grep -v sources`

AGENT_PATH=
AGENT_PATH=`find target/ -name "*.so"`
if [ not $AGENT_PATH ]; then
  AGENT_PATH=`find target/ -name "*.jnilib"`
fi

$JAVA_HOME/bin/java -cp $JAVA_HOME/lib/tools.jar:$PAN_GRABBER_JAR_FILE com.github.marschall.pangrabber.Attacher -a $AGENT_PATH

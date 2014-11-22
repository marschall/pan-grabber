package com.github.marschall.pangrabber;

import static java.nio.charset.StandardCharsets.US_ASCII;
import static java.nio.file.StandardOpenOption.WRITE;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Collections;
import java.util.List;
import java.util.Properties;

import com.sun.tools.attach.AgentInitializationException;
import com.sun.tools.attach.AgentLoadException;
import com.sun.tools.attach.AttachNotSupportedException;
import com.sun.tools.attach.VirtualMachine;
import com.sun.tools.attach.VirtualMachineDescriptor;

public class Attacher {

  public static void main(String[] args) throws IOException {
    // https://github.com/giltene/jHiccup
    ExceptionCounterConfiguration configuration;
    try {
      configuration = new ExceptionCounterConfiguration(args);
    } catch (ConfigurationException e) {
      System.err.println(e.getMessage());
      usage(System.err);
      System.exit(1);
      return;
    }

    List<VirtualMachineDescriptor> descriptors;
    try {
      descriptors = getVmDescriptors(configuration);
    } catch (ConfigurationException e) {
      System.err.println(e.getMessage());
      System.exit(0);
      return; // keep compiler happy
    }
    
    Path channel = Paths.get("channel").toAbsolutePath();
    try {
      grabPansInVms(configuration, descriptors, channel);
    } finally {
      Files.deleteIfExists(channel);
    }
    System.exit(0);
  }

  public static void grabPansInVms(ExceptionCounterConfiguration configuration,
      List<VirtualMachineDescriptor> descriptors, Path channel) {
    for (VirtualMachineDescriptor descriptor : descriptors) {
      try {
        grabPansInVm(configuration, descriptor, channel);
      } catch (AttachNotSupportedException | AgentLoadException | AgentInitializationException | IOException  e) {
        System.err.println("failed to attach");
      }
    }
  }

  public static void grabPansInVm(ExceptionCounterConfiguration configuration,
      VirtualMachineDescriptor descriptor, Path channel) throws AttachNotSupportedException,
      IOException, AgentLoadException, AgentInitializationException {
    if (Files.exists(channel)) {
      try (SeekableByteChannel byteChannel = Files.newByteChannel(channel, WRITE)) {
        byteChannel.truncate(0L);
      }
    } else {
      Files.createFile(channel);
    }
    
    // TODO check if provider supports attaching
    VirtualMachine vm = VirtualMachine.attach(descriptor);
    Properties systemProperties = vm.getSystemProperties();
    String javaCommand = systemProperties.getProperty("sun.java.command");
    if (javaCommand == null || javaCommand.startsWith(Attacher.class.getName())) {
      return;
    }
    vm.loadAgentPath(configuration.agentPath, channel.toString());
    vm.detach();
    
    try (BufferedReader reader = Files.newBufferedReader(channel, US_ASCII)) {
      String line;
      while ((line = reader.readLine()) != null) {
        System.out.println(line);
      }
    }
  }
  
  private static List<VirtualMachineDescriptor> getVmDescriptors(ExceptionCounterConfiguration configuration) throws ConfigurationException {
    List<VirtualMachineDescriptor> descriptors = VirtualMachine.list();
    String pid = configuration.pidOfProcessToAttachTo;
    if (pid == null) {
      return descriptors;
    } else {
      for (VirtualMachineDescriptor descriptor : descriptors) {
        if (descriptor.id().equals(pid)) {
          return Collections.singletonList(descriptor);
        }
      }
      throw new ConfigurationException("not process with pid: " + pid + " found");
    }
  }

  private static void usage(PrintStream s) {
    String validArgs =
        "\"[-p pidOfProcessToAttachTo] [-a agentPath]\"\n";

    s.println("valid arguments = " + validArgs);

    s.println(
        " [-h]                        help\n"
        + " [-p pidOfProcessToAttachTo] Attach to the process with given pid and inject exception counter as an agent."
            + " If missing attache to all processes but this one\n"
        + " [-a agentPath] Absolute patch to the exception counter agent binary.\n");

  }

  static final class ExceptionCounterConfiguration {

    String pidOfProcessToAttachTo;
    String agentPath;

    public ExceptionCounterConfiguration(String[] args) throws ConfigurationException {
      for (int i = 0; i < args.length; ++i) {
        if (args[i].equals("-p")) {
          if (args.length >= i) {
            pidOfProcessToAttachTo = args[++i];
          } else {
            throw new ConfigurationException("Missing value for -p");
          }
        } else if (args[i].equals("-a")) {
          if (args.length >= i) {
            agentPath = args[++i];
          } else {
            throw new ConfigurationException("Missing value for -a");
          }
        } else {
          throw new ConfigurationException("Invalid args: " + args[i]);
        }
      }

      if (agentPath == null) {
        throw new ConfigurationException("Missing argument -a");
      }

    }

  }

  static final class ConfigurationException extends Exception {

    ConfigurationException(String message) {
      super(message);
    }

  }

}

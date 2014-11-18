package com.github.marschall.pangrabber;

import java.io.Console;

public class SampleApplication {
  
  // All PANs from http://www.getcreditcardnumbers.com/
  private static final String STATIC_PAN = "5131401258231639";

  public static void main(String[] args) {
    String[] panArray = {"5277753060160834", "5265084775874053", "5304830015633045"};
    String dynamicPan = "559935969757" + "7498";
    Console console = System.console();
    console.readLine();
  }

}

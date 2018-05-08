import java.net.*;
import java.io.*;
import java.nio.*;
import java.lang.*;

public class HardwareDevice implements Runnable {
  private Socket socket;
  private OutputStream outStream;
  private InputStream inStream;

  public void startConnection(String ip, int port) throws UnknownHostException {
    try {
      socket = new Socket(ip, port);
      outStream = socket.getOutputStream();
      inStream = socket.getInputStream();
    }
    catch (Exception e) {
      System.out.println("Failed to create connection to server");
    }
  }

  public byte[] bufferInt (int x) {

    ByteBuffer buf = ByteBuffer.allocate(4);
    buf.order(ByteOrder.LITTLE_ENDIAN);
    buf.putInt(x);
    return buf.array();

  }

  public double currentTime () {

    return (double)System.currentTimeMillis() / 1000.0;

  }

  public void run () {

    System.out.println("Inside of thead");

    double time = currentTime();
    double passed = 0.0;
    int i = 0;
    while (true) {

      double snapshot = currentTime();

      if (currentTime() - time >= 3.0) {
        try {
          outStream.write(bufferInt(1));
          outStream.write(bufferInt(16));
          outStream.write(bufferInt(50+i));
          outStream.write(bufferInt(50+i));
          outStream.write(bufferInt(50+i));
          outStream.write(bufferInt(50+i));
          outStream.flush();
          time = currentTime();
          i++;
        }
        catch (Exception e) {
          System.out.println("Failed to send stats");
        }
      }

      else if (passed >= 10.0) {
        try {
          String fname = "./array0.txt";
          String name = "array0.txt";
          File fText = new File(fname);
          FileInputStream f = new FileInputStream(fname);
          byte[] data = new byte[(int)fText.length()];
          f.read(data);

          outStream.write(bufferInt(0));
          outStream.write(bufferInt(16+(int)fText.length()+name.length()));
          outStream.write(bufferInt(5));
          outStream.write(bufferInt(5));
          outStream.write(bufferInt(name.length()));
          outStream.write(name.getBytes());
          outStream.write(bufferInt((int)fText.length()));
          outStream.write(data);
          outStream.flush();

          System.out.println("Sent result file to server");
          passed = 0.0;
        }
        catch (Exception e) {
          System.out.println("Unable to send result file to server");
        }
      }

      passed += currentTime() - snapshot;

    }


  }

  public void device () {

    try {
      startConnection("128.2.13.145", 15744);
    }
    catch (Exception e) {
      return;
    }

    System.out.println("Now connected to server");

    try {
      outStream.write(bufferInt(2));
      outStream.flush();

      outStream.write(bufferInt(23));
      outStream.write(bufferInt(11));
      outStream.write(bufferInt(82));
      outStream.write(bufferInt(76));
      outStream.flush();
    }
    catch (Exception e) {
      System.out.println("Failed to send data");
      return;
    }

    System.out.println("Sent data");

    Thread sendNode = new Thread(this, "SendNode");
    sendNode.start();

    System.out.println("Launched thread to send data to server");

    while (true) {

      try {
      ByteBuffer buf;
      byte[] temp;

      byte[] initialSize = new byte[4];
      inStream.read(initialSize, 0, 4);
      buf = ByteBuffer.wrap(initialSize);
      buf.order(ByteOrder.LITTLE_ENDIAN);
      int arraySize = buf.getInt();

      System.out.println("Array size = "+arraySize);

      byte[] array = new byte[arraySize];
      inStream.read(array, 0, arraySize);
      buf = ByteBuffer.wrap(array);
      buf.order(ByteOrder.LITTLE_ENDIAN);

      int jobID = buf.getInt();
      int exeID = buf.getInt();

      System.out.println("jobID = "+jobID+" exeID = "+exeID);

      int exeNameSize = buf.getInt();

      temp = new byte[exeNameSize];
      buf.get(temp, 0, exeNameSize);
      String exeName = new String(temp);

      System.out.println("exeName = "+exeName);

      int exeDataSize = buf.getInt();

      System.out.println("exe data size = "+exeDataSize);

      byte[] exeData = new byte[exeDataSize];
      buf.get(exeData, 0, exeDataSize);

      int numberTextFiles = buf.getInt();

      System.out.println("number of text files = "+numberTextFiles);

      for (int i = 0; i < numberTextFiles; i++) {
        int textNameSize = buf.getInt();

        temp = new byte[textNameSize];
        buf.get(temp, 0, textNameSize);
        String textName = new String(temp);

        System.out.println("textName = "+textName);

        int textDataSize = buf.getInt();

        System.out.println("data size = "+textDataSize);

        byte[] textData = new byte[textDataSize];
        buf.get(textData, 0, textDataSize);

        FileOutputStream fText = new FileOutputStream("./"+textName);
        fText.write(textData);
      }

      FileOutputStream fExe = new FileOutputStream("./"+exeName);
      fExe.write(exeData);

      Runtime.getRuntime().exec("chmod +x Map");
      System.out.println("Changed permissions on exe");
      //Runtime.getRuntime().exec("./Map");
      //System.out.println("Executed exe");

      }
      catch (Exception e ) {
        System.out.println("Unable to read data");
      }

    }

  }

  public static void main (String args[]) throws Exception {

    HardwareDevice client = new HardwareDevice();
    client.device();

  }
}

package subscriber;

import java.io.*;
import java.net.*;
import java.time.chrono.IsoChronology;
import java.util.Scanner;

public class Subscriber {
    public  static int          BUFFER_SIZE = 1024;
    private static int          messageID = 0;
    private static final String clientID = "abcdjfu";

    private Socket              connFdListen;
    private Socket              connFdSub;
    private InputStream         in;
    private OutputStream        out;
    private String              serverAddress;
    private int                 serverPort;

    public Subscriber(){
        this.serverAddress = "localhost";
        this.serverPort    = 1608;
    }

    public int getServerPort() {
        return serverPort;
    }

    public String getServerAddress() {
        return serverAddress;
    }

    public static String getClientID() {
        return clientID;
    }

    public void subscribe(){

        String location;
        String sensor;
        String[] topics= new String[BUFFER_SIZE];
        int len = 0;
        while(true){
            System.out.println("Press @ for subscribe");
            Scanner inputUser =  new Scanner(System.in);
            System.out.print("Subscribe to topic :");
            sensor = inputUser.nextLine();
            if( sensor.equals("@"))
                break;
            System.out.print("Subscribe to location: ");
            location = inputUser.nextLine();
            if( sensor.equals("@"))
                break;
            topics[len++] = PacketMessage.makeTopic(location, sensor);
        }
        try{
            this.connFdSub = new Socket(getServerAddress(), getServerPort());
            System.out.println("Connectd to server :" + this.connFdSub);
            
            InputStream in = connFdSub.getInputStream();
            OutputStream out  = connFdSub.getOutputStream();
            byte[] buff = new byte[BUFFER_SIZE];
            int bytes;

            bytes = PacketMessage.makeSubscribe(buff,getMessageID(),getClientID(), topics, len);
            out.write(buff, 0, bytes);
            incMessageID();

            System.out.println("Sent SUBSCRIBE");

            if( (bytes= in.read(buff)) != -1){
                System.out.println("Received SUBACK");
            }

        } catch (SocketException ex){

        }catch (IOException ex){
            System.out.println(ex.fillInStackTrace());
        }
    }

    public void start(){
        try{
            this.connFdListen = new Socket(getServerAddress(), getServerPort());
            ListenThread listenThread= new ListenThread(connFdListen);
            listenThread.start();
            System.out.println("Connected to server " + connFdListen);

            while(true){
                subscribe();
            }
        } catch (SocketException ex){

        } catch (IOException ex){
            System.out.println(ex.fillInStackTrace());
        }
    }

    public synchronized int getMessageID() {
        return messageID;
    }

    public synchronized void setMessageID(int messageID){
        this.messageID = messageID;
    }

    public synchronized void incMessageID(){
        this.messageID++;
    }

    public static void main(String[] argv){
        Subscriber subscriber = new Subscriber();
        subscriber.start();
    }



    class ListenThread extends Thread{

        public static final int BUFFER_SIZE = 1024;
        private Socket          connFd;
        private InputStream     in;
        private OutputStream    out;

        public ListenThread(Socket connFd){
            this.connFd = connFd;
        }

        public void run(){
            try{
                in = connFd.getInputStream();
                out = connFd.getOutputStream();
                byte[] recvBuff = new byte[BUFFER_SIZE];
                byte[] sentBuff = new byte[BUFFER_SIZE];

                int n_read = 0, n_write = 0;
                System.out.println("Listen publish at " + connFd);

                // sent connect pkt to server
                n_write = PacketMessage.makeConnect(sentBuff,getClientID());
                out.write(sentBuff, 0, n_write);

                System.out.println("Sent CONNECT");

                while(true){
                    if( (n_read = in.read(recvBuff, 0, recvBuff.length)) != -1){
                        n_write = 0;
                        switch (PacketMessage.getMessageType(recvBuff)){
                            case 0: // nothing
                                break;
                            case 1: // connect
                                break;
                            case 2: // connack
                                System.out.println("Received CONNACK");
                                System.out.println("MessageID   :" + PacketMessage.getMessageId(recvBuff, PacketMessage.FIXED_HEADER_SIZE));
                                System.out.println("Return code :" + PacketMessage.recvConnack(recvBuff));
                                break;
                            case 3: // publish
                                String[] msg = PacketMessage.recvPublish(recvBuff, n_read);
                                System.out.println("Received PUBLISH");
                                System.out.println("MessageID:" + msg[0]);
                                System.out.println("Topic name:" + msg[1]);
                                System.out.println("Payload:" + msg[2]);

                                n_write = PacketMessage.makePuback(sentBuff, getMessageID());
                                incMessageID();
                                break;
                            case 4: // puback
                                break;
                            case 5: // pubrec
                                break;
                            case 6: // pubrel
                                break;
                            case 7: // pubcomp
                                break;
                            case 8: // subcribe
                                break;
                            case 9: // suback
                                break;
                        }
                        if( n_write > 0)
                            out.write(sentBuff, 0, n_write);
                    }
                }

            } catch (SocketException ex){

            }
            catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}

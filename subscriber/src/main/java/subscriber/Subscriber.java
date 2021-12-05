package subscriber;

import java.io.*;
import java.net.*;

public class Subscriber {

    private static int          messageID = 0;

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

    public void start(){

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



    class listenThread extends Thread{

        public static final int BUFFER_SIZE = 1024;
        private Socket          connFd;
        private InputStream     in;
        private OutputStream    out;

        public listenThread(Socket connFd){
            this.connFd = connFd;
        }

        public void run(){
            try{
                in = connFd.getInputStream();
                out = connFd.getOutputStream();
                System.out.println("Listen publish at " + connFd);

                byte[] recvBuff = new byte[BUFFER_SIZE];
                byte[] sentBuff = new byte[BUFFER_SIZE];

                int n_read = 0, n_write = 0;
                while(true){
                    if( (n_read = in.read(recvBuff, 0, recvBuff.length)) != -1){
                        switch (PacketMessage.getMessageType(recvBuff)){
                            case 0: // nothing
                                break;
                            case 1: // connect
                                break;
                            case 2: // connack
                                break;
                            case 3: // publish
                                String[] msg = PacketMessage.recvPublish(recvBuff, n_read);
                                System.out.println("MessageID :" + msg[0]);
                                System.out.println("Topic name:" + msg[1]);
                                System.out.println("Payload   :" + msg[2]);

                                n_write = PacketMessage.makePuback(sentBuff, getMessageID());
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

import java.io.*;
import java.net.*;
import java.util.Scanner;

public class Sensor {

    public static final int BUFFER_SIZE                 = 1024;
    public static final int FIXED_HEADER_SIZE           = 5;
    public static final int REMAINING_LENGTH_SIZE       = 4;
    public static final int TOPIC_NAME_LENGTH_SIZE      = 2;
    public static final int MESSAGE_ID_SIZE             = 2;
    public static final int PAYLOAD_LENGTH_SIZE         = 2;

    public static int       messageID = 0;
    private Socket          connFd;
    private InputStream     in;
    private OutputStream    out;
    private String          brokerAddress;
    private int             brokerPort;
    private String          sensorName;

    public Sensor(){
        this.brokerAddress = "localhost";
        this.brokerPort = 1608;
    }

    public Sensor(String sensorName){
        this.brokerAddress = "localhost";
        this.brokerPort = 1608;
        this.sensorName = sensorName;
    }

    public String getSensorName() {
        return sensorName;
    }

    private void setRemainingLength(byte[] pkt, int remainingLength){
        for( int i = 0, idx = 1; i < REMAINING_LENGTH_SIZE; i++){
            pkt[idx++] = (byte)(remainingLength & 0xff);
            remainingLength >>= 8;
        }
    }
    private void setFixedHeader(byte[] pkt, int msgType, int dupFlag, int qosLevel, int retain,int remainingLength){
        pkt[0] = (byte)(msgType << 4 | dupFlag << 3 | qosLevel << 1 | retain);
        setRemainingLength(pkt, remainingLength);
    }

    private int setTopicLength(byte[] pkt, int idx, int length){
        for(int i = 0; i < TOPIC_NAME_LENGTH_SIZE; i++){
            pkt[idx++] = (byte)(length & 0xff);
            length >>= 8;
        }
        return idx;
    }

    private int setTopicName(byte[] pkt, int idx, String topicName){
        for(int i = 0; i <topicName.length(); i++)
            pkt[idx++] = (byte)topicName.charAt(i);
        return idx;
    }

    private int setTopic(byte[] pkt, int idx, String location){
        String topic = '/' + location + '/' + getSensorName();
        idx = setTopicLength(pkt, idx, topic.length());
        return setTopicName(pkt, idx, topic);
    }

    private int setMessageID(byte[] pkt, int idx){
        int msgID = messageID;
        for(int i = 0; i < MESSAGE_ID_SIZE; i++){
            pkt[idx++] = (byte)(msgID & 0xff);
            msgID >>= 8;
        }
        return idx;
    }

    private int setPayloadLength(byte[] pkt, int idx, int length){
        for( int i = 0; i < PAYLOAD_LENGTH_SIZE; i++){
            pkt[idx++] = (byte)(length & 0xff);
            length >>= 8;
        }
        return idx;
    }

    private int setPayload(byte[] pkt, int idx, String payload){
        idx = setPayloadLength(pkt, idx, payload.length());
        for( int i = 0; i < payload.length(); i++)
            pkt[idx++] = (byte)payload.charAt(i);
        return idx;
    }

    /// /location/sensor/payload
    private int makePublish(byte[] pkt, String location, String payload){
        int idx = FIXED_HEADER_SIZE;
        idx = setMessageID(pkt, idx);
        idx = setTopic(pkt, idx, location);
        idx = setPayload(pkt, idx, payload);
        setFixedHeader(pkt, 3, 0, 1, 0, idx - FIXED_HEADER_SIZE);
        return idx;
    }

    private int getMessageType(byte[] pkt){
        return pkt[0] >> 4;
    }

    private int getMessageID(byte[] pkt, int idx){
        int msgID = 0;
        for( int i = idx + MESSAGE_ID_SIZE - 1; i >= idx; i++)
            msgID = msgID << 4 + pkt[i];
        return msgID;
    }

    public void start(){
        try{
            //this.connFd = new Socket(this.brokerAddress, this.brokerPort);
            //this.in = connFd.getInputStream();
            //this.out = connFd.getOutputStream();

            byte[] sentBuff = new byte[BUFFER_SIZE];
            byte[] recvBuff = new byte[BUFFER_SIZE];
            int n_read, n_write;
            System.out.println("Connected to server " + connFd);

            /** make publish msg sent to broker
                for test
            */
            Scanner scanner = new Scanner(System.in);
            while(true){
                String location, payload;

                System.out.print("location: ");
                location = scanner.nextLine();
                System.out.print("payload: ");
                payload = scanner.nextLine();
                n_write = makePublish(sentBuff, location, payload);
                out.write(sentBuff, 0, n_write);

                if( (n_read = in.read(recvBuff)) != -1){
                    messageID = getMessageID(recvBuff, FIXED_HEADER_SIZE);
                };
            }
        } catch(SocketException ex){

        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] argv){
        String sensorName;
        sensorName = (argv.length == 1) ? argv[0] : "sensorA";
        Sensor sensor = new Sensor(sensorName);
        sensor.start();
    }
}

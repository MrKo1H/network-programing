
import java.util.Optional;

public class PacketMessage {

    public static final int BUFFER_SIZE                 = 1024;
    public static final int FIXED_HEADER_SIZE           = 5;
    public static final int REMAINING_LENGTH_SIZE       = 4;
    public static final int TOPIC_NAME_LENGTH_SIZE      = 2;
    public static final int MESSAGE_ID_SIZE             = 2;
    public static final int PAYLOAD_LENGTH_SIZE         = 2;
    public static final int CLIENT_ID_SIZE              = 23;

    public static void setRemainingLength(byte[] pkt, int length){
        for( int i = 0, idx = 1; i < REMAINING_LENGTH_SIZE; i++){
            pkt[idx++] = (byte) (length & 0xff);
            length >>= 8;
        }
    }
    public static void setFixedHeader(byte[]pkt, int msgType, int remainingLength){
        pkt[0] = (byte)(msgType << 4);
        setRemainingLength(pkt, remainingLength);
    }

    public static int getMessageType(byte[] pkt){
        return pkt[0] >> 4;
    }

    public static int getDupFlag(byte[] pkt){
        return pkt[0] >> 3 & 1;
    }
    public static int getQosLevel(byte[] pkt){
        return pkt[0] >> 1 & 0b11;
    }

    public static int getRetain(byte[] pkt){
        return pkt[0] & 0b1;
    }

    public static int getRemainingLength(byte[] pkt){
        int idx = REMAINING_LENGTH_SIZE, length = 0;
        for(int i = REMAINING_LENGTH_SIZE;i > 0;  i--)
            length = length << 8 | pkt[idx--];
        return length;
    }

    public static int setConnectFlag(byte[] pkt,int idx, int username, int password, int willRetain, int willQos, int willFlag, int cleanSession){
        pkt[idx++] = (byte)(username << 7 | password << 6 | willRetain << 5 | willQos << 3 | willFlag << 2 | cleanSession << 1);
        return idx;
    }
    public static int setClientID(byte[] pkt, int idx, String clientID){
        for(int i = 0; i < CLIENT_ID_SIZE; i++)
            pkt[idx++] = (byte)( clientID.length() > i ? clientID.charAt(i) : '\00');
        return idx;
    }

    public static int setMessageId(byte[] pkt, int idx, int messageID){
        for(int i = 0; i < MESSAGE_ID_SIZE; i++){
            pkt[idx++] = (byte)(messageID & 0xff);
            messageID >>= 8;
        }
        return idx;
    }

    public static int getMessageId(byte[] pkt, int idx){
        int msgId = 0;
        for(int i = idx + MESSAGE_ID_SIZE - 1; i >= idx; i--)
            msgId = msgId << 8 | pkt[i];
        return msgId;
    }

    public static int setTopicLength(byte[] pkt, int idx, int length){
        for( int i = 0; i < TOPIC_NAME_LENGTH_SIZE; i++){
            pkt[idx++] = (byte)(length & 0xff);
            length >>= 8;
        }
        return idx;
    }

    public static int getTopicLength(byte[] pkt, int idx){
        int len = 0;
        for( int i = idx + TOPIC_NAME_LENGTH_SIZE - 1; i >= idx; i--)
            len = len << 8 | pkt[i];
        return len;
    }
    public static int setTopicName(byte[] pkt, int idx, String topicName){
        for( int i = 0; i < topicName.length();i ++)
            pkt[idx++] = (byte)topicName.charAt(i);
        return idx;
    }

    public static String getTopicName(char[] pkt, int idx, int len){
        String topic = "";
        for( int i = 0; i < len; i++)
            topic += (char)pkt[idx++];
        return topic;
    }

    public static int setTopic(byte[] pkt, int idx, String topic){
        idx = setTopicLength(pkt, idx, topic.length());
        return setTopicName(pkt, idx, topic);
    }

    public static int getPayloadLength(byte[] pkt, int idx){
        int len = 0;
        for(int i = idx + PAYLOAD_LENGTH_SIZE - 1; i >= idx; i--)
            len = len << 8 | pkt[i];
        return len;
    }

    public static String getPayload(byte[] pkt, int idx, int len){
        String payload = "";
        for(int i = 0; i < len; i++)
            payload += (char)pkt[idx++];
        return payload;
    }

    public static String makeTopic(String location, String sensor){
        return '/' + location + '/' + sensor;
    }

    /** connect pkt sent to broker
     * */
    public static int makeConnect(byte[] pkt, String clientID){
        int idx = FIXED_HEADER_SIZE;
        idx = setConnectFlag(pkt, idx, 0,0,0,0,0,0);
        idx = setClientID(pkt, idx, clientID);
        setFixedHeader(pkt, 1, idx - FIXED_HEADER_SIZE);
        return idx;
    }

    /** connack packet received from broker
     * */
    public static int recvConnack(byte[] pkt){
        return pkt[FIXED_HEADER_SIZE];
    }
    /** received pkt publish from brker
     * */

    public static String[] recvPublish(byte[] pkt, int len){
        int idx = FIXED_HEADER_SIZE;
        int lenTopic, lenPayload;
        String topicName = "", payload = "";
        int msgID;

        msgID = getMessageId(pkt, idx);
        idx += MESSAGE_ID_SIZE;

        lenTopic = getTopicLength(pkt, idx);
        idx += TOPIC_NAME_LENGTH_SIZE;
        for(int i = 0; i < lenTopic; i++)
            topicName += (char) pkt[idx++];

        lenPayload = getPayloadLength(pkt, idx);
        idx += PAYLOAD_LENGTH_SIZE;
        payload = getPayload(pkt, idx, lenPayload);

        return new String[]{String.valueOf(msgID), topicName, payload};
    }
    /** puback response to server
     * */
    public static int makePuback(byte[] pkt, int messageID){
        int idx = FIXED_HEADER_SIZE;
        idx = setMessageId(pkt, idx, messageID);
        setFixedHeader(pkt, 4, idx - FIXED_HEADER_SIZE);
        return idx;
    }


    /** subscribe pkt sent to server
     * */
    public static int makeSubscribe(byte[] pkt , int messageID,String clientID, String[] topics, int len){
        int idx = FIXED_HEADER_SIZE;
        idx = setMessageId(pkt, idx, messageID);
        idx = setClientID(pkt, idx, clientID);
        for( int i = 0; i < len; i++)
            idx = setTopic(pkt, idx, topics[i]);
        setFixedHeader(pkt, 8,idx - FIXED_HEADER_SIZE);
        return idx;
    }
    public static int makeUnsubscribe(byte[] pkt , int messageID,String clientID, String[] topics, int len){
        int idx = FIXED_HEADER_SIZE;
        idx = setMessageId(pkt, idx, messageID);
        idx = setClientID(pkt, idx, clientID);
        for( int i = 0; i < len; i++)
            idx = setTopic(pkt, idx, topics[i]);
        setFixedHeader(pkt, 5,idx - FIXED_HEADER_SIZE);
        return idx;
    }

    public static void suback(){}
}

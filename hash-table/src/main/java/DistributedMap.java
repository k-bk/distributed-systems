import org.jgroups.JChannel;
import org.jgroups.ReceiverAdapter;
import org.jgroups.protocols.*;
import org.jgroups.protocols.pbcast.GMS;
import org.jgroups.protocols.pbcast.NAKACK2;
import org.jgroups.protocols.pbcast.STABLE;
import org.jgroups.stack.ProtocolStack;
import org.jgroups.util.Util;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

public class DistributedMap extends ReceiverAdapter implements SimpleStringMap {

    private final Map<String, Integer> map;
    private JChannel channel;

    public DistributedMap(String cluster_name) throws Exception {
        map = new HashMap<>();
        channel = new JChannel(false);

        ProtocolStack stack = new ProtocolStack();
        channel.setProtocolStack(stack);
        stack
                .addProtocol(new UDP())
                .addProtocol(new PING())
                .addProtocol(new MERGE3())
                .addProtocol(new FD_SOCK())
                .addProtocol(new FD_ALL().setValue("timeout", 12000).setValue("interval", 3000))
                .addProtocol(new VERIFY_SUSPECT())
                .addProtocol(new BARRIER())
                .addProtocol(new NAKACK2())
                .addProtocol(new UNICAST3())
                .addProtocol(new STABLE())
                .addProtocol(new GMS())
                .addProtocol(new UFC())
                .addProtocol(new MFC())
                .addProtocol(new FRAG2());
        stack.init();

        System.out.println(" connecting to a channel...");
        channel.connect(cluster_name, null, 0);
    }

    public void delete() {
        System.out.println("  closing channel...");
        channel.close();
    }

    public void getState(OutputStream output) throws Exception {
        synchronized(map) {
            Util.objectToStream(map, new DataOutputStream(output));
        }
    }

    public void setState(InputStream input) throws Exception {
        Map<String, Integer> new_map = (HashMap<String, Integer>) Util.objectFromStream(new DataInputStream(input));
        synchronized(map) {
            map.clear();
            map.putAll(new_map);
        }
    }

    @Override
    public boolean containsKey(String key) {
        synchronized (map) {
            return map.containsKey(key);
        }
    }

    @Override
    public Integer get(String key) {
        synchronized (map) {
            return map.get(key);
        }
    }

    @Override
    public void put(String key, Integer value) {
        synchronized (map) {
            map.put(key, value);
            channel.getState();
        }
    }

    @Override
    public Integer remove(String key) {
        synchronized (map) {
            Integer result =  map.remove(key);
            if (result != null) {
                channel.getState();
            }
            return result;
        }
    }
}

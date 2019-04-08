import org.jgroups.*;
import org.jgroups.protocols.*;
import org.jgroups.protocols.pbcast.*;
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
    private final String cluster_name;

    public DistributedMap(String cluster_name) throws Exception {
        this.map = new HashMap<>();
        this.cluster_name = cluster_name;
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
                .addProtocol(new FRAG2())
                .addProtocol(new STATE_TRANSFER())
                .addProtocol(new DISCARD())
                .addProtocol(new FLUSH());

        stack.init();
        channel.setReceiver(this);
        channel.connect(cluster_name, null, 0);
    }

    public void receive(Message msg) {
        String line = (String) msg.getObject();
        String[] tokens = line.split(" ");
        synchronized (map) {
            switch (tokens[0]) {
                case "put":
                    map.put(tokens[1], Integer.parseInt(tokens[2]));
                    break;
                case "remove":
                    map.remove(tokens[1]);
                    break;
            }
        }
    }

    public void viewAccepted(View view) {
        if (view instanceof MergeView) {
            ViewHandler handler  = new ViewHandler(channel, (MergeView) view);
            handler.start();
        }
    }

    private static class ViewHandler extends Thread {
        JChannel channel;
        MergeView view;

        private ViewHandler(JChannel channel, MergeView view) {
            this.channel = channel;
            this.view = view;
        }

        public void run() {
            View tmp_view = view.getSubgroups().get(0);
            Address local_address = channel.getAddress();
            if(!tmp_view.getMembers().contains(local_address)) {
                System.out.println("\n  merge: not coordinator, reacquiring the state (" + tmp_view + ")\n");
                try {
                    channel.getState(null, 3000);

                } catch (Exception ignored) {
                    // ignored
                }
            } else {
                System.out.println("\n  merge: coordinator, do nothing (" + tmp_view + ")\n");
            }
        }
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
        printAll();
    }

    public void printAll() {
        System.out.println(" " + map.size() + " entries in a hash map:");
        for (Map.Entry<String, Integer> e : map.entrySet()) {
            System.out.println("  " + e.getKey() + " : " + e.getValue());
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
            send("put " + key + " " + value);
        }
    }

    @Override
    public Integer remove(String key) {
        synchronized (map) {
            Integer result =  map.remove(key);
            if (result != null) {
                send("remove " + key);
            }
            return result;
        }
    }

    private void send(String message) {
        try {
            channel.send(new Message(null, null, message));
        } catch (Exception e) {
            System.out.println("  error: unable to send.");
        }
    }
}

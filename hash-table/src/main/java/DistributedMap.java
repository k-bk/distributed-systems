import java.util.HashMap;
import java.util.Map;

public class DistributedMap implements SimpleStringMap {

    private Map<String, Integer> map;

    public DistributedMap() {
        map = new HashMap<>();
    }

    @Override
    public boolean containsKey(String key) {
        synchronized (map) {
            return map.containsKey(key);
        };
    }

    @Override
    public Integer get(String key) {
        synchronized (map) {
            return map.get(key);
        };
    }

    @Override
    public void put(String key, Integer value) {
        synchronized (map) {
            map.put(key, value);
        };
    }

    @Override
    public Integer remove(String key) {
        synchronized (map) {
            map.remove(key);
        };
    }
}

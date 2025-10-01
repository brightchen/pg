package bc.monitor;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class DeviceEventTest {

    private final ObjectMapper objectMapper = new ObjectMapper();

    @Test
    void testEventCreation() {
        DeviceEvent event = new DeviceEvent("e123", "d456", 1234567890L);

        assertThat(event.getEventId()).isEqualTo("e123");
        assertThat(event.getDeviceId()).isEqualTo("d456");
        assertThat(event.getTimestamp()).isEqualTo(1234567890L);
        assertThat(event.getEventGroup()).isEqualTo("d456");
    }

    @Test
    void testJsonSerialization() throws Exception {
        DeviceEvent event = new DeviceEvent("e123", "d456", 1234567890L);

        String json = objectMapper.writeValueAsString(event);

        assertThat(json).contains("\"event-id\":\"e123\"");
        assertThat(json).contains("\"device-id\":\"d456\"");
        assertThat(json).contains("\"time\":1234567890");
    }

    @Test
    void testJsonDeserialization() throws Exception {
        String json = "{\"event-id\":\"e123\",\"device-id\":\"d456\",\"time\":1234567890}";

        DeviceEvent event = objectMapper.readValue(json, DeviceEvent.class);

        assertThat(event.getEventId()).isEqualTo("e123");
        assertThat(event.getDeviceId()).isEqualTo("d456");
        assertThat(event.getTimestamp()).isEqualTo(1234567890L);
    }

    @Test
    void testEqualsAndHashCode() {
        DeviceEvent event1 = new DeviceEvent("e123", "d456", 1234567890L);
        DeviceEvent event2 = new DeviceEvent("e123", "d456", 1234567890L);
        DeviceEvent event3 = new DeviceEvent("e999", "d456", 1234567890L);

        assertThat(event1).isEqualTo(event2);
        assertThat(event1.hashCode()).isEqualTo(event2.hashCode());
        assertThat(event1).isNotEqualTo(event3);
    }

    @Test
    void testToString() {
        DeviceEvent event = new DeviceEvent("e123", "d456", 1234567890L);

        String str = event.toString();

        assertThat(str).contains("e123");
        assertThat(str).contains("d456");
        assertThat(str).contains("1234567890");
    }
}

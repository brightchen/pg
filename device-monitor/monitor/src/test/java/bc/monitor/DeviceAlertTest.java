package bc.monitor;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class DeviceAlertTest {

    private final ObjectMapper objectMapper = new ObjectMapper();

    @Test
    void testAlertCreation() {
        DeviceAlert alert = new DeviceAlert("d456", 1234567890L);

        assertThat(alert.getDeviceId()).isEqualTo("d456");
        assertThat(alert.getLatestEventTime()).isEqualTo(1234567890L);
    }

    @Test
    void testJsonSerialization() throws Exception {
        DeviceAlert alert = new DeviceAlert("d456", 1234567890L);

        String json = objectMapper.writeValueAsString(alert);

        assertThat(json).contains("\"device-id\":\"d456\"");
        assertThat(json).contains("\"latest-event-time\":1234567890");
    }

    @Test
    void testJsonDeserialization() throws Exception {
        String json = "{\"device-id\":\"d456\",\"latest-event-time\":1234567890}";

        DeviceAlert alert = objectMapper.readValue(json, DeviceAlert.class);

        assertThat(alert.getDeviceId()).isEqualTo("d456");
        assertThat(alert.getLatestEventTime()).isEqualTo(1234567890L);
    }

    @Test
    void testEqualsAndHashCode() {
        DeviceAlert alert1 = new DeviceAlert("d456", 1234567890L);
        DeviceAlert alert2 = new DeviceAlert("d456", 1234567890L);
        DeviceAlert alert3 = new DeviceAlert("d999", 1234567890L);

        assertThat(alert1).isEqualTo(alert2);
        assertThat(alert1.hashCode()).isEqualTo(alert2.hashCode());
        assertThat(alert1).isNotEqualTo(alert3);
    }

    @Test
    void testToString() {
        DeviceAlert alert = new DeviceAlert("d456", 1234567890L);

        String str = alert.toString();

        assertThat(str).contains("d456");
        assertThat(str).contains("1234567890");
    }
}

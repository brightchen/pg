package bc.eventgen;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Represents an IoT device event.
 */
public class DeviceEvent {

    @JsonProperty("event-id")
    private final String eventId;

    @JsonProperty("device-id")
    private final String deviceId;

    @JsonProperty("time")
    private final long timestamp;

    public DeviceEvent(String eventId, String deviceId, long timestamp) {
        this.eventId = eventId;
        this.deviceId = deviceId;
        this.timestamp = timestamp;
    }

    public String getEventId() {
        return eventId;
    }

    public String getDeviceId() {
        return deviceId;
    }

    public long getTimestamp() {
        return timestamp;
    }

    @Override
    public String toString() {
        return "DeviceEvent{" +
                "eventId='" + eventId + '\'' +
                ", deviceId='" + deviceId + '\'' +
                ", timestamp=" + timestamp +
                '}';
    }
}

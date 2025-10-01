package bc.monitor;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonCreator;

import java.util.Objects;

/**
 * Implementation of Event for IoT device events.
 * Maps device-id as the event group.
 */
public class DeviceEvent implements Event {

    @JsonProperty("event-id")
    private final String eventId;

    @JsonProperty("device-id")
    private final String deviceId;

    @JsonProperty("time")
    private final long timestamp;

    @JsonCreator
    public DeviceEvent(
            @JsonProperty("event-id") String eventId,
            @JsonProperty("device-id") String deviceId,
            @JsonProperty("time") long timestamp) {
        this.eventId = eventId;
        this.deviceId = deviceId;
        this.timestamp = timestamp;
    }

    @Override
    @JsonIgnore
    public long getTimestamp() {
        return timestamp;
    }

    @Override
    @JsonIgnore
    public String getEventId() {
        return eventId;
    }

    @Override
    @JsonIgnore
    public String getEventGroup() {
        return deviceId;
    }

    @JsonIgnore
    public String getDeviceId() {
        return deviceId;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        DeviceEvent that = (DeviceEvent) o;
        return timestamp == that.timestamp &&
                Objects.equals(eventId, that.eventId) &&
                Objects.equals(deviceId, that.deviceId);
    }

    @Override
    public int hashCode() {
        return Objects.hash(eventId, deviceId, timestamp);
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

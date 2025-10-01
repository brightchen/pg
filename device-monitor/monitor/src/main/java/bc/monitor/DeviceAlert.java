package bc.monitor;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonCreator;

import java.util.Objects;

/**
 * Alert message for inactive devices.
 */
public class DeviceAlert {

    @JsonProperty("device-id")
    private final String deviceId;

    @JsonProperty("latest-event-time")
    private final long latestEventTime;

    @JsonCreator
    public DeviceAlert(
            @JsonProperty("device-id") String deviceId,
            @JsonProperty("latest-event-time") long latestEventTime) {
        this.deviceId = deviceId;
        this.latestEventTime = latestEventTime;
    }

    public String getDeviceId() {
        return deviceId;
    }

    public long getLatestEventTime() {
        return latestEventTime;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        DeviceAlert that = (DeviceAlert) o;
        return latestEventTime == that.latestEventTime &&
                Objects.equals(deviceId, that.deviceId);
    }

    @Override
    public int hashCode() {
        return Objects.hash(deviceId, latestEventTime);
    }

    @Override
    public String toString() {
        return "DeviceAlert{" +
                "deviceId='" + deviceId + '\'' +
                ", latestEventTime=" + latestEventTime +
                '}';
    }
}

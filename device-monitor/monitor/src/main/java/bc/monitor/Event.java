package bc.monitor;

import java.io.Serializable;

/**
 * Abstract interface for events in the monitoring system.
 */
public interface Event extends Serializable {

    /**
     * @return The timestamp of the event as epoch time in milliseconds
     */
    long getTimestamp();

    /**
     * @return The unique event identifier
     */
    String getEventId();

    /**
     * @return The event group identifier (e.g., device-id for IoT devices)
     */
    String getEventGroup();
}

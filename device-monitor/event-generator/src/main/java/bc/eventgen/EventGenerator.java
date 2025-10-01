package bc.eventgen;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.ProducerConfig;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.StringSerializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Generates mock IoT device events and publishes them to Kafka.
 *
 * Usage:
 *   java -jar event-generator.jar --kafka.bootstrap.servers=localhost:9092 \
 *                                   --num.devices=10 \
 *                                   --event.interval.ms=10 \
 *                                   --duration.seconds=60
 */
public class EventGenerator {

    private static final Logger LOG = LoggerFactory.getLogger(EventGenerator.class);
    private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper();

    private final String bootstrapServers;
    private final String topicName;
    private final int numDevices;
    private final long eventIntervalMs;
    private final long durationSeconds;
    private final double failureProbability;

    private final AtomicBoolean running = new AtomicBoolean(true);
    private final AtomicLong eventCounter = new AtomicLong(0);

    public EventGenerator(String bootstrapServers, String topicName, int numDevices,
                          long eventIntervalMs, long durationSeconds, double failureProbability) {
        this.bootstrapServers = bootstrapServers;
        this.topicName = topicName;
        this.numDevices = numDevices;
        this.eventIntervalMs = eventIntervalMs;
        this.durationSeconds = durationSeconds;
        this.failureProbability = failureProbability;
    }

    public static void main(String[] args) {
        String bootstrapServers = getParameter(args, "kafka.bootstrap.servers", "localhost:9092");
        String topicName = getParameter(args, "topic", "events");
        int numDevices = Integer.parseInt(getParameter(args, "num.devices", "10"));
        long eventIntervalMs = Long.parseLong(getParameter(args, "event.interval.ms", "10"));
        long durationSeconds = Long.parseLong(getParameter(args, "duration.seconds", "60"));
        double failureProbability = Double.parseDouble(getParameter(args, "failure.probability", "0.1"));

        LOG.info("Starting Event Generator with parameters:");
        LOG.info("  kafka.bootstrap.servers: {}", bootstrapServers);
        LOG.info("  topic: {}", topicName);
        LOG.info("  num.devices: {}", numDevices);
        LOG.info("  event.interval.ms: {}", eventIntervalMs);
        LOG.info("  duration.seconds: {}", durationSeconds);
        LOG.info("  failure.probability: {}", failureProbability);

        EventGenerator generator = new EventGenerator(
                bootstrapServers, topicName, numDevices, eventIntervalMs,
                durationSeconds, failureProbability);

        // Setup shutdown hook
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            LOG.info("Shutting down...");
            generator.stop();
        }));

        try {
            generator.run();
        } catch (Exception e) {
            LOG.error("Error running generator", e);
            System.exit(1);
        }
    }

    public void run() throws Exception {
        Properties props = new Properties();
        props.put(ProducerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        props.put(ProducerConfig.KEY_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());
        props.put(ProducerConfig.VALUE_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());
        props.put(ProducerConfig.ACKS_CONFIG, "1");
        props.put(ProducerConfig.RETRIES_CONFIG, 3);

        try (KafkaProducer<String, String> producer = new KafkaProducer<>(props)) {
            LOG.info("Started producing events...");

            long startTime = System.currentTimeMillis();
            long endTime = startTime + (durationSeconds * 1000);
            Random random = new Random();

            // Track active devices (some may "fail" and stop sending events)
            Set<String> activeDevices = new HashSet<>();
            for (int i = 0; i < numDevices; i++) {
                activeDevices.add("d" + i);
            }

            while (running.get() && System.currentTimeMillis() < endTime) {
                long iterationStart = System.currentTimeMillis();

                // Randomly fail/recover devices
                List<String> deviceList = new ArrayList<>(activeDevices);
                for (String deviceId : new ArrayList<>(activeDevices)) {
                    if (random.nextDouble() < failureProbability / 100) {
                        activeDevices.remove(deviceId);
                        LOG.info("Device {} stopped sending events", deviceId);
                    }
                }

                // Try to recover failed devices
                for (int i = 0; i < numDevices; i++) {
                    String deviceId = "d" + i;
                    if (!activeDevices.contains(deviceId) && random.nextDouble() < 0.01) {
                        activeDevices.add(deviceId);
                        LOG.info("Device {} recovered", deviceId);
                    }
                }

                // Generate events for active devices
                for (String deviceId : activeDevices) {
                    long eventId = eventCounter.incrementAndGet();
                    long timestamp = System.currentTimeMillis();

                    DeviceEvent event = new DeviceEvent("e" + eventId, deviceId, timestamp);
                    String json = OBJECT_MAPPER.writeValueAsString(event);

                    ProducerRecord<String, String> record = new ProducerRecord<>(
                            topicName, deviceId, json);

                    producer.send(record, (metadata, exception) -> {
                        if (exception != null) {
                            LOG.error("Error sending event: {}", event, exception);
                        }
                    });
                }

                // Log progress
                if (eventCounter.get() % 1000 == 0) {
                    long elapsed = System.currentTimeMillis() - startTime;
                    double rate = eventCounter.get() / (elapsed / 1000.0);
                    LOG.info("Generated {} events, rate: {:.2f} events/sec, active devices: {}",
                            eventCounter.get(), rate, activeDevices.size());
                }

                // Sleep to maintain the desired event rate
                long iterationTime = System.currentTimeMillis() - iterationStart;
                long sleepTime = eventIntervalMs - iterationTime;
                if (sleepTime > 0) {
                    Thread.sleep(sleepTime);
                }
            }

            LOG.info("Finished. Generated {} events total", eventCounter.get());
        }
    }

    public void stop() {
        running.set(false);
    }

    private static String getParameter(String[] args, String key, String defaultValue) {
        for (String arg : args) {
            if (arg.startsWith("--" + key + "=")) {
                return arg.substring(("--" + key + "=").length());
            }
        }
        return defaultValue;
    }
}

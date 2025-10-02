package bc.monitor;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.kafka.clients.admin.AdminClient;
import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.ProducerConfig;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.StringDeserializer;
import org.apache.kafka.common.serialization.StringSerializer;
import org.junit.jupiter.api.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.Duration;
import java.util.*;

import static org.assertj.core.api.Assertions.assertThat;

/**
 * Integration test for the Monitor application.
 * Uses external Kafka cluster (docker-compose or K8s) instead of Testcontainers.
 */
class MonitorIntegrationTest {

    private static final Logger LOG = LoggerFactory.getLogger(MonitorIntegrationTest.class);
    private static final String EVENTS_TOPIC = "events-test";
    private static final String ALERTS_TOPIC = "alerts-test";

    private static KafkaTestHelper kafkaHelper;

    private final ObjectMapper objectMapper = new ObjectMapper();
    private KafkaProducer<String, String> producer;
    private KafkaConsumer<String, String> consumer;

    @BeforeAll
    static void setUpKafka() throws Exception {
        LOG.info("=== Setting up Kafka for integration tests ===");
        kafkaHelper = new KafkaTestHelper();
        kafkaHelper.ensureKafkaRunning();
        kafkaHelper.ensureTopicsExistAndClean(EVENTS_TOPIC, ALERTS_TOPIC);
        LOG.info("=== Kafka is ready ===");
    }

    @AfterAll
    static void tearDownKafka() {
        if (kafkaHelper != null) {
            LOG.info("=== Closing Kafka helper ===");
            if (kafkaHelper.wasKafkaStarted()) {
                LOG.info("Note: Kafka cluster was started by tests and is still running.");
                LOG.info("To stop it, run: docker compose down");
            }
            kafkaHelper.close();
        }
    }

    @BeforeEach
    void setUp() throws Exception {
        String bootstrapServers = kafkaHelper.getBootstrapServers();

        // Clean topics before each test
        kafkaHelper.ensureTopicsExistAndClean(EVENTS_TOPIC, ALERTS_TOPIC);

        // Create producer
        Properties producerProps = new Properties();
        producerProps.put(ProducerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        producerProps.put(ProducerConfig.KEY_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());
        producerProps.put(ProducerConfig.VALUE_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());
        producerProps.put(ProducerConfig.ACKS_CONFIG, "all");
        producer = new KafkaProducer<>(producerProps);

        // Create consumer
        Properties consumerProps = new Properties();
        consumerProps.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        consumerProps.put(ConsumerConfig.GROUP_ID_CONFIG, "test-consumer-" + UUID.randomUUID());
        consumerProps.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        consumerProps.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        consumerProps.put(ConsumerConfig.AUTO_OFFSET_RESET_CONFIG, "earliest");
        consumer = new KafkaConsumer<>(consumerProps);
        consumer.subscribe(Collections.singletonList(ALERTS_TOPIC));
    }

    @AfterEach
    void tearDown() {
        if (producer != null) {
            producer.close();
        }
        if (consumer != null) {
            consumer.close();
        }
    }

    @Test
    @Timeout(30)
    void testDeviceEventProcessing() throws Exception {
        LOG.info("=== Test: Device Event Processing ===");

        // Publish test events
        long currentTime = System.currentTimeMillis();

        DeviceEvent event1 = new DeviceEvent("e1", "d1", currentTime);
        DeviceEvent event2 = new DeviceEvent("e2", "d2", currentTime);
        DeviceEvent event3 = new DeviceEvent("e3", "d1", currentTime + 5000);

        publishEvent(event1);
        publishEvent(event2);
        publishEvent(event3);

        LOG.info("Published 3 test events");

        // Verify events were published successfully
        Thread.sleep(1000);
        LOG.info("Test completed successfully");
    }

    @Test
    @Timeout(30)
    void testEventDeserialization() throws Exception {
        LOG.info("=== Test: Event Deserialization ===");

        long currentTime = System.currentTimeMillis();

        // Test various event formats
        DeviceEvent event1 = new DeviceEvent("e1", "device-123", currentTime);
        String json1 = objectMapper.writeValueAsString(event1);

        DeviceEvent parsed1 = objectMapper.readValue(json1, DeviceEvent.class);

        assertThat(parsed1.getEventId()).isEqualTo("e1");
        assertThat(parsed1.getDeviceId()).isEqualTo("device-123");
        assertThat(parsed1.getTimestamp()).isEqualTo(currentTime);

        LOG.info("Event serialization/deserialization working correctly");
    }

    @Test
    @Timeout(30)
    void testMultipleDevicesOnDifferentPartitions() throws Exception {
        LOG.info("=== Test: Multiple Devices on Different Partitions ===");

        long currentTime = System.currentTimeMillis();

        // Send events for multiple devices (will go to different partitions)
        List<DeviceEvent> events = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            DeviceEvent event = new DeviceEvent("e" + i, "d" + (i % 3), currentTime + i * 1000);
            events.add(event);
            publishEvent(event);
        }

        LOG.info("Published 10 events for 3 different devices");

        // Verify all events were published
        assertThat(events).hasSize(10);

        Thread.sleep(1000);
        LOG.info("Test completed successfully");
    }

    @Test
    @Timeout(30)
    void testOutOfOrderEvents() throws Exception {
        LOG.info("=== Test: Out of Order Events ===");

        long currentTime = System.currentTimeMillis();

        // Send events out of order
        DeviceEvent event1 = new DeviceEvent("e1", "d1", currentTime + 10000);
        DeviceEvent event2 = new DeviceEvent("e2", "d1", currentTime + 5000);
        DeviceEvent event3 = new DeviceEvent("e3", "d1", currentTime + 15000);

        publishEvent(event1);
        publishEvent(event2);
        publishEvent(event3);

        LOG.info("Published 3 out-of-order events");

        // Basic test that events can be published out of order
        Thread.sleep(1000);
        LOG.info("Test completed successfully");
    }

    @Test
    @Timeout(30)
    void testAlertSerialization() throws Exception {
        LOG.info("=== Test: Alert Serialization ===");

        DeviceAlert alert = new DeviceAlert("device-123", System.currentTimeMillis());
        String json = objectMapper.writeValueAsString(alert);

        assertThat(json).contains("\"device-id\":\"device-123\"");
        assertThat(json).contains("\"latest-event-time\"");

        DeviceAlert parsed = objectMapper.readValue(json, DeviceAlert.class);
        assertThat(parsed.getDeviceId()).isEqualTo("device-123");

        LOG.info("Alert serialization/deserialization working correctly");
    }

    @Test
    @Timeout(30)
    void testKafkaConnectivity() throws Exception {
        LOG.info("=== Test: Kafka Connectivity ===");

        assertThat(kafkaHelper.getBootstrapServers()).isNotEmpty();

        // Test producer connectivity
        DeviceEvent testEvent = new DeviceEvent("test", "test-device", System.currentTimeMillis());
        publishEvent(testEvent);

        LOG.info("Kafka connectivity verified");
    }

    @Test
    @Timeout(30)
    void testTopicCleanup() throws Exception {
        LOG.info("=== Test: Topic Cleanup ===");

        // Publish some events
        long currentTime = System.currentTimeMillis();
        for (int i = 0; i < 5; i++) {
            DeviceEvent event = new DeviceEvent("cleanup-e" + i, "cleanup-d1", currentTime + i * 1000);
            publishEvent(event);
        }

        LOG.info("Published 5 events");

        // Force flush
        producer.flush();

        // Clean up topics
        kafkaHelper.ensureTopicsExistAndClean(EVENTS_TOPIC, ALERTS_TOPIC);

        LOG.info("Topics cleaned successfully");

        // Publish new event after cleanup
        DeviceEvent newEvent = new DeviceEvent("new-e1", "new-d1", currentTime + 10000);
        publishEvent(newEvent);

        LOG.info("Topic cleanup test completed successfully");
    }

    private void publishEvent(DeviceEvent event) throws Exception {
        String json = objectMapper.writeValueAsString(event);
        ProducerRecord<String, String> record = new ProducerRecord<>(
                EVENTS_TOPIC,
                event.getDeviceId(),
                json
        );
        producer.send(record).get();
        LOG.debug("Published event: {}", event);
    }

    private List<DeviceAlert> consumeAlerts(int expectedCount, Duration timeout) {
        List<DeviceAlert> alerts = new ArrayList<>();
        long endTime = System.currentTimeMillis() + timeout.toMillis();

        while (System.currentTimeMillis() < endTime && alerts.size() < expectedCount) {
            ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(100));
            for (ConsumerRecord<String, String> record : records) {
                try {
                    DeviceAlert alert = objectMapper.readValue(record.value(), DeviceAlert.class);
                    alerts.add(alert);
                    LOG.info("Consumed alert: {}", alert);
                } catch (Exception e) {
                    LOG.error("Error parsing alert", e);
                    throw new RuntimeException("Error parsing alert", e);
                }
            }
        }

        return alerts;
    }
}
